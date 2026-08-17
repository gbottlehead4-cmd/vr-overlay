// VisorVR site Worker.
//
// Serves the static site from `site/` via the ASSETS binding, and handles the
// one dynamic route, /api/hits.
//
// This started as a Cloudflare Pages Function. Cloudflare has retired Pages
// project creation, so the same logic now runs as a Worker with static assets:
// the routing that Pages did by convention (functions/api/hits.js -> /api/hits)
// is explicit here instead.
//
// Requires a KV namespace bound as COUNTER; see wrangler.jsonc.

const REPO = "gbottlehead4-cmd/vr-overlay";

/** Visit tally + cached GitHub download total. */
async function hits(env) {
  const KV = env.COUNTER;

  // --- visit tally (read-modify-write; fine at this traffic level) ---
  let visits = parseInt((await KV.get("visits")) || "0", 10) || 0;
  visits += 1;
  await KV.put("visits", String(visits));

  // --- download total, cached 10 min ---
  let downloads = await KV.get("downloads_cache");
  if (downloads === null) {
    try {
      const res = await fetch(`https://api.github.com/repos/${REPO}/releases`, {
        headers: {
          "User-Agent": "visorvr-site",
          Accept: "application/vnd.github+json",
        },
      });
      const rels = await res.json();
      let n = 0;
      for (const rel of Array.isArray(rels) ? rels : []) {
        for (const a of rel.assets || []) n += a.download_count || 0;
      }
      downloads = String(n);
      // keep the last good value 24h as a fallback if GitHub is later unreachable
      await KV.put("downloads_cache", downloads, { expirationTtl: 600 });
      await KV.put("downloads_last", downloads, { expirationTtl: 86400 });
    } catch (e) {
      downloads = (await KV.get("downloads_last")) || "0";
    }
  }

  return new Response(
    JSON.stringify({ visits, downloads: parseInt(downloads, 10) || 0 }),
    {
      headers: {
        "content-type": "application/json",
        "cache-control": "no-store",
      },
    },
  );
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    if (url.pathname === "/api/hits") {
      if (request.method !== "GET") {
        return new Response("Method Not Allowed", { status: 405 });
      }
      // A missing or misconfigured KV binding must not take the site down: the
      // footer's stats line hides itself when this call fails.
      if (!env.COUNTER) {
        return new Response(JSON.stringify({ error: "no KV binding" }), {
          status: 503,
          headers: { "content-type": "application/json" },
        });
      }
      try {
        return await hits(env);
      } catch (e) {
        return new Response(JSON.stringify({ error: String(e) }), {
          status: 500,
          headers: { "content-type": "application/json" },
        });
      }
    }

    // Everything else is a static file from site/.
    return env.ASSETS.fetch(request);
  },
};
