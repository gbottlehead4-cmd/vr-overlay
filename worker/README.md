# VisorVR site Worker

Serves the landing page in `site/` as static assets, plus one dynamic route.

This was a Cloudflare **Pages** project until Cloudflare retired Pages project
creation — the dashboard now only offers Workers. `worker/index.js` does the
routing Pages used to do by filename convention.

## `/api/hits` — visit + download counter

Powers the small stats line in the site footer (`👁 … visits · ⬇ … downloads`).

- **visits** — a page-load tally stored in Cloudflare KV (this site's own count).
- **downloads** — total GitHub release-asset downloads for the repo, cached in KV
  for 10 minutes so visitors never call GitHub's API directly. Reads 0 while the
  repo is private; the footer then shows visits only.

If the KV binding is missing the route returns 503 and the footer line hides
itself. The rest of the page is unaffected.

## One-time setup

The Worker needs a KV namespace bound as **`COUNTER`**. Create it and paste the
id into `wrangler.jsonc`:

```
npx wrangler kv namespace create COUNTER
```

Or via the dashboard: **Workers & Pages → KV → Create a namespace** (name it
e.g. `visorvr-site-counter`), then copy its ID.

Until `wrangler.jsonc` has a real id in place of `PASTE_KV_NAMESPACE_ID_HERE`,
deploys fail — deliberately. A loud failure beats a site that quietly serves a
broken counter.

## Deploying

The Cloudflare dashboard's Workers Builds runs `npx wrangler deploy` on every
push to the production branch. To deploy by hand:

```
npx wrangler deploy
```

## Notes

- The name in `wrangler.jsonc` decides the URL
  (`visorvr.gbottlehead4.workers.dev`). The site hardcodes that in its canonical
  link, `og:url`, `sitemap.xml` and `robots.txt` — renaming means updating those
  four places too.
- `site/` holds only static files. Nothing there is built or bundled, so there
  is no build command.
