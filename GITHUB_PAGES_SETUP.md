# GitHub Pages Setup (Free Hosting)

This repository is configured to deploy the landing page to GitHub Pages using Actions.

## What is already configured

- Workflow: `.github/workflows/pages.yml`
- Published files:
  - `index.html`
  - `TECHNICAL_ASSESSMENT.md`
  - `DUE_DILIGENCE_PACK.md`
  - `PROVENANCE_AUDIT.md`
  - `COMMERCIAL_LICENSE_AGREEMENT.md`
  - `docs/DETERMINISM.md`
  - `docs/BENCHMARK_BASELINES.md`
- Custom domain file: `CNAME` with `upe.bearpackonlineservices.com`

## One-time GitHub steps

1. Push to `main`.
2. Open repository Settings -> Pages.
3. Under Build and deployment, set Source to `GitHub Actions`.
4. Wait for the `Deploy Landing Page to GitHub Pages` workflow to finish.

## DNS setup for custom domain

In your DNS provider for `bearpackonlineservices.com`:

- Create `CNAME` record:
  - Host/Name: `upe`
  - Target/Value: `<your-github-username>.github.io`

If your DNS provider does not allow CNAME flattening for subdomains, standard CNAME still works for this case.

## Verification

After DNS propagates, verify both URLs:

- `https://<your-github-username>.github.io/<repo-name>/`
- `https://upe.bearpackonlineservices.com/`

## Notes

- Every push to `main` that changes the listed landing page files triggers a fresh deployment.
- If you rename the repository or branch, update `.github/workflows/pages.yml` as needed.
- If you do not want a custom domain yet, remove `CNAME`.
