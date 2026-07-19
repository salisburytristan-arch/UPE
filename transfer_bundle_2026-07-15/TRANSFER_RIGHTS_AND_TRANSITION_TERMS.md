# Transfer Rights and Transition Terms (Deal Template)

Date: 2026-07-15
Project: Unified Physics Engine (UPE)
Purpose: Define transfer-ready rights language and transition support scope for definitive agreement drafting.

## 1) Rights Grant (Subject to Signed Definitive Agreement)

Upon closing and payment under the definitive transaction documents, Buyer receives the right to:

1. Use the project in any internal or external commercial context.
2. Modify, extend, and create derivative works.
3. Rebrand and rename the project, including replacement of marks and public identity.
4. Commercialize and distribute products incorporating the project.
5. Sublicense granted rights to affiliates, customers, and partners (as negotiated in final agreement).
6. Assign or transfer rights in connection with M&A or corporate restructuring (as negotiated).
7. Discontinue, archive, or sunset the project at Buyer's discretion.

All rights are subject to any third-party obligations explicitly listed in final schedules.

## 2) Ownership and IP Chain-of-Title

Seller represents (in final agreement) that:

1. Seller has authority to transfer the rights being sold.
2. Seller owns or controls the contributed code/assets included in transfer scope, except as disclosed.
3. Any excluded assets, encumbrances, or third-party constraints are listed in disclosure schedules.

Supporting diligence files in repo:
- `PROVENANCE_AUDIT.md`
- `docs/PROVENANCE_AND_OWNERSHIP.md`
- `IP_CHAIN_OF_TITLE_CHECKLIST.md`

## 3) Included IP and Brand Assets

Intended transfer scope includes, if negotiated and listed in final schedules:

1. Source code and repository history.
2. Project name and branding assets.
3. Domain name and website content.
4. Documentation, tests, benchmarks, and demo assets.
5. Build/deployment scripts and tooling.
6. In-scope datasets and generated non-third-party artifacts.

Excluded by default unless explicitly listed in signed schedules:

1. Seller parent-company brands, trade names, trademarks, and business identities not specific to UPE.
2. Seller-wide payment processor, checkout, or merchant accounts.
3. Seller-wide hosting, DNS, and infrastructure accounts unrelated to UPE-only operations.
4. Any non-UPE products, repositories, domains, or customer contracts.

## 4) Transition Support Period

Recommended baseline transition support:

1. Duration: 30 calendar days after closing.
2. Capacity: 10 total support hours (aggregate cap).
3. Channels: email plus one architecture walkthrough session.
4. Scope:
   - repository import and build troubleshooting
   - one architecture walkthrough
   - transfer-blocker triage for packaging/import issues only
5. Exclusions:
   - net-new feature development
   - custom integrations not present in transfer scope
   - performance tuning beyond basic build sanity
   - 24/7 SLA or on-call support

Optional extension:
- Additional services only by prepaid statement of work at mutually agreed hourly rate.

## 5) Transfer Conditions and Completion

Transfer completion milestones:

1. Delivery of archives and integrity hashes.
2. Delivery/transfer of in-scope accounts and credentials through secure channel.
3. Confirmation of repository import and successful build by Buyer.
4. Sign-off on transition handoff checklist.

## 6) Legal Finalization Note

This file is an operational template for diligence and negotiation.
Binding rights and obligations arise only from fully executed definitive agreements.
