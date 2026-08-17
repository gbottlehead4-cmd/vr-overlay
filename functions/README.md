# Cloudflare Pages Functions

## `/api/hits` — visit + download counter

Powers the small stats line in the site footer (`👁 … visits · ⬇ … downloads`).

- **visits** — a page-load tally stored in Cloudflare KV (this site's own count).
- **downloads** — total GitHub release-asset downloads for the repo, cached in KV
  for 10 minutes so visitors never call GitHub's API directly. Reads 0 while the
  repo is private; the footer then shows visits only.

### One-time setup (Cloudflare dashboard)

The function needs a KV namespace bound as **`COUNTER`**:

1. Cloudflare dashboard → **Workers & Pages → KV** → **Create a namespace**
   (name it e.g. `visorvr-site-counter`).
2. Open the **Pages** project (`visorvr`) → **Settings → Bindings →
   KV namespace bindings** → **Add binding**.
   - Variable name: `COUNTER`
   - KV namespace: the one from step 1
   - Add it for **Production** (and Preview if you use preview deploys).
3. Redeploy (any push to the deploy branch, or **Deployments → Retry** on the latest).

Until the binding exists the function errors and the footer line just stays
hidden — the rest of the page is unaffected.

### Notes

- The `functions/` folder MUST sit at the **repo root**, not inside `site/` —
  Cloudflare Pages discovers Functions from the project root, not the static
  output dir. So `functions/api/hits.js` routes to `/api/hits`.
- KV limits writes to the same key to ~1/second, so a burst of simultaneous
  visits may miss one. Fine for a vanity counter; Cloudflare Web Analytics has
  the authoritative numbers.
- Reset the count by deleting the `visits` key in the KV namespace.
