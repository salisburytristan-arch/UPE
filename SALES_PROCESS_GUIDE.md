# Sales Process Guide: NDA → Diligence → Acquisition

This guide explains how to present the Unified Physics Engine to potential buyers, starting with the NDA.

---

## Phase 1: Initial Outreach (Pre-NDA)

### 1.1 Identify Qualified Buyers

Target companies that would benefit from physics engine IP:
- Game engine studios (Godot, custom engines)
- Enterprise simulation platforms
- VFX/animation software providers
- Robotics & autonomous vehicle companies
- Research institutions
- Visual effects studios

### 1.2 Send Executive Summary

Share the **EXECUTIVE_SUMMARY.md** and **QUICK_START.md** (no IP exposure yet):
- 1-page headline: "Deterministic physics core, 55k LOC, zero dependencies"
- Feature overview (rigid bodies, vehicles, articulated, cloth, fluids)
- Timeline: "Ready for immediate integration"
- Request: "Are you interested in a confidential technical evaluation?"

### 1.3 Schedule Discovery Call (30 min)

If buyer expresses interest:
- Confirm decision-makers and timeline
- Discuss use case fit
- Explain pre-evaluation requirements (NDA, technical lead assignment, budget)
- Set next steps: "We'll send the NDA for your legal team to review"

---

## Phase 2: NDA Execution (Week 1)

### 2.1 Customize & Send NDA

1. **Download**: `NDA_TEMPLATE.md`
2. **Customize**:
   - Insert your company name and contact info
   - Insert buyer company name and representative details
   - Set evaluation period: typically 30-90 days
   - Choose governing law jurisdiction (your home jurisdiction is simplest)
3. **Convert to PDF**: Use any markdown-to-PDF tool or save as PDF directly
4. **Send via email**: Include cover letter below

**Sample NDA Cover Email:**

```
Subject: Confidential Evaluation Agreement – Unified Physics Engine v1.0.0

Dear [Buyer Legal Contact],

We are pleased to move forward with a confidential technical evaluation of our proprietary 
Unified Physics Engine for [Buyer Company Name].

Attached is our standard Non-Disclosure Agreement (NDA). Please have your legal team 
review and sign both pages. Key points:

- Evaluation Period: [XX days] from execution
- Permitted Use: Technical assessment for acquisition/licensing consideration only
- Security: You agree to secure storage and restrict access to need-to-know personnel
- Survival: Confidentiality obligations survive [XX years] after termination
- No Obligation: Signing does not commit either party to a transaction

Once we receive the fully signed NDA, we will immediately provide:
- Diligence package (source code, docs, examples, proofs)
- Build instructions and quickstart guide
- Our technical contact for integration questions

Please return the signed agreement to [your email] within 5 business days if possible.

Looking forward to working with your team.

Best regards,
[Your Name]
[Your Title]
[Your Company]
[Your Phone]
```

### 2.2 Legal Review Process

- Buyer's legal team reviews for ~5-10 days
- Standard requests:
  - Shorten evaluation period (you can agree to 60-90 days)
  - Loosen IP restrictions (politely decline; offer "residual knowledge" clause already included)
  - Change indemnification (you can negotiate here if needed)
- Typical outcome: 2-3 rounds of minor edits, then signature

### 2.3 Track Signature

- **Milestone**: Both parties sign and date all pages
- **Archival**: Scan and store signed original in secure folder
- **Send Confirmation**: "Thank you for signing. We're sending the diligence package now."

---

## Phase 3: Diligence Package Delivery (Week 1)

### 3.1 Prepare the Package

Right after NDA is signed, send:

1. **Access info email** with:
   - Download link to: `UnifiedPhysicsEngine-1.0.0-buyout-2026-02-26.zip` (0.93 MB)
   - SHA256 hash of the ZIP file (for integrity verification)
   - Unzip instructions & folder structure overview

2. **Included in package**:
   - Complete source code (core, examples, tests, benchmarks)
   - All 19 buyer-facing docs
   - CMake build system
   - Proof artifacts (determinism logs, test results)
   - Build scripts (Windows `.bat`, Linux `.sh`)

3. **Supplementary documents** (sent separately or in package):
   - **BUYER_START_HERE.md** ← Start here (15-min quickstart)
   - **DUE_DILIGENCE_PACK.md** ← Structured evaluation path (2-4 weeks)
   - **BUYER_EVALUATION_CHECKLIST.md** ← Step-by-step technical validation
   - **TECHNICAL_ASSESSMENT.md** ← Architecture & risk analysis

### 3.2 Kick-off Call

Within 2 days of sending package, schedule **Technical Kickoff (1 hour)**:

**Attendees**:
- Buyer: Technical lead, architect, decision-maker
- You: Yourself (or tech person who knows the engine)

**Agenda**:
1. (5 min) Confirm they received the package
2. (10 min) Walk through BUYER_START_HERE.md structure
3. (15 min) Live demo: Show build process, run example, display test results
4. (20 min) Answer technical questions (scope, APIs, limitations)
5. (10 min) Set up communication channel (email, weekly calls, Slack)

**Key talking points**:
- "Determinism is proven—validated with replay/hash tooling"
- "Zero dependencies: pure C stdlib, works on Windows/Linux/macOS"
- "60+ unit tests, comprehensive examples, full CMake build system"
- "You can build and validate everything in 1-2 hours"
- "We provide implementation details, not black boxes"

---

## Phase 4: Technical Evaluation (Weeks 2-4)

### 4.1 Buyer's Evaluation Timeline

Typically buyer follows this path:

| Week | Activity | Deliverable |
|------|----------|-------------|
| 1 | Build system, run tests, read API reference | Build success, test results, initial questions |
| 2 | Benchmark, architecture review, code walkthrough | Performance notes, technical questions |
| 3 | Integration mockup, evaluate feasibility, risk assessment | Integration plan draft, go/no-go recommendation |
| 4 | Final review, executive summary, decision | Acquisition interest level |

### 4.2 Your Support Role

- **Availability**: Respond to technical questions within 24 hours
- **Honesty**: Answer truthfully about limitations (no gatekeeping on weaknesses)
- **Guidance**: Point them to relevant docs but let them validate independently
- **Escalation**: If exec/legal questions arise, involve your team

**Sample Q&A responses**:

| Q | A |
|---|---|
| "Where's the GPU support?" | "GPU compute is not in v1.0.0 scope. Currently CPU-optimized with SIMD. See KNOWN_LIMITATIONS.md." |
| "Can we modify the solver?" | "Yes, solver is modular. See ARCHITECTURE.md for extension points. Source code is provided." |
| "What about performance at 100k bodies?" | "Validated to ~10k bodies in our tests. Larger scales require custom broadphase tuning. See benchmark logs in proof/." |
| "How long to integrate into our engine?" | "Typically 2-6 weeks depending on your engine architecture. See INTEGRATION_GUIDE.md for patterns." |

### 4.3 Weekly Sync Calls (Optional)

- **Frequency**: Every Friday at [agreed time], 30 min
- **Format**: Brief tech update + Q&A + next week's focus
- **Tone**: Collaborative, not defensive. You want them to make an informed decision.

---

## Phase 5: Acquisition Decision (Week 4-5)

### 5.1 Buyer's Go/No-Go Decision

Buyer presents recommendation:

**GO**: "We want to acquire / license this engine"  
→ Proceed to Phase 6 (Negotiation)

**CONDITIONAL**: "Interested, but we need X changes / lower price / support commitment"  
→ Negotiate, re-evaluate, set new deadline

**NO-GO**: "Not a good fit for our roadmap"  
→ Thank them, archive NDA/DOCs, move to next buyer

### 5.2 Your Response to GO

1. **Schedule executive call** with buyer's C-level + your team
2. **Discuss terms**:
   - Acquisition (you transfer all IP and exit)
   - Licensing (you retain IP, they pay per-deployment)
   - Subscription (you support and update, they pay annually)
3. **Outline next steps**:
   - Term sheet / Letter of Intent (LOI)
   - Full due diligence (financial, IP, legal)
   - Closing in 30-90 days

---

## Phase 6: Negotiation & Closing

### 6.1 Structure the Deal

**Option A: Outright Acquisition**
- Buyer pays fixed sum → they own all IP
- You: Walk away with money, no ongoing support
- Buyer: Full control, can modify, sublicense, etc.
- Timeline: 60-90 days to close
- Price: Usually $[TBD based on buyer size/budget]

**Option B: Exclusive License**
- Buyer pays upfront + annual fee → they have exclusive rights in their industry
- You: Retain ownership, receive passive income
- Buyer: Can modify, but cannot sub-license to competitors
- Timeline: 30-60 days to close
- Price: Lower upfront (1-2M range sample), plus annual recurring

**Option C: Support & Maintenance Contract**
- Buyer owns the IP, but you provide 1-2 years of integration support
- Bundled with acquisition or licensing
- Timeline: Negotiated separately
- Price: $[K per month for dedicated engineer time]

### 6.2 Get Legal Support

- **Hire M&A lawyer** in buyer's jurisdiction (if international)
- **Review term sheet** before responding
- **Negotiate**:
  - IP warranties (you warrant clear chain-of-title)
  - Indemnification (you cover IP infringement claims)
  - Non-compete (you agree not to sell competitive engine for X years)
  - Support (duration, SLA, availability)

### 6.3 Typical Deal Timeline

```
Week 1: Initial LOI and term sheet exchange
Week 2-3: Buyer's full diligence (financial, IP, compliance)
Week 4-5: Legal drafting of definitive agreement
Week 6-8: Final negotiations, board approvals
Week 9-10: Closing (payment, IP transfer, signatures)

Total: ~60-90 days from first "GO" to cash
```

---

## Phase 7: After Signing

### 7.1 IP Transfer

If acquisition:
- **Transfer all source repositories**
- **Provide all documentation, build artifacts, proof logs**
- **Transfer domain names, if any** (physics-engine.com, etc.)
- **Sign IP assignment agreement** (part of closing docs)

### 7.2 Knowledge Transfer

- **2-week technical onboarding** (if agreed in contract)
- **Code walkthrough**: Major systems, extension points, internal APIs
- **Turnover debugging notes**: Known issues, workarounds, rough edges
- **Build/deploy guide**: How to set up CI/CD, cross-compile, etc.

### 7.3 Transition

- **Archive your development**: Delete local repos, old build artifacts
- **Provide final documentation**: README for handoff, legacy issues list
- **Final sign-off**: Confirm buyer can build/run independently

---

## Quick Checklist: What to Send When

### Pre-NDA
- [ ] EXECUTIVE_SUMMARY.md
- [ ] QUICK_START.md

### With NDA Cover Email
- [ ] NDA_TEMPLATE.md (as PDF)

### Right After NDA Signed
- [ ] UnifiedPhysicsEngine-1.0.0-buyout-*.zip
- [ ] SHA256 hash file
- [ ] BUYER_START_HERE.md
- [ ] DUE_DILIGENCE_PACK.md
- [ ] BUYER_EVALUATION_CHECKLIST.md
- [ ] TECHNICAL_ASSESSMENT.md

### At Technical Kickoff
- [ ] Verbal walkthrough of above docs
- [ ] Build demo + live example run
- [ ] Contact info, weekly sync schedule

### Upon Buyer Request (Week 2-3)
- [ ] Custom integration demo (if time permits)
- [ ] Clarifications on algorithms, performance data
- [ ] Binding effect or liability info (escalate to legal)

### After GO Decision
- [ ] Executive summary for buyer's board
- [ ] Financial model (if licensing, not acquisition)
- [ ] Support & maintenance SLA proposal

---

## Key Tips

1. **Be transparent**: Answer questions directly, don't oversell or hedge
2. **Provide proof**: Benchmarks, test results, and determinism validation are evidence
3. **Respect NDA**: Never share details with competing potential buyers during evaluation
4. **Document everything**: Keep records of all communications, signed NDAs, key decisions
5. **Involve legal early**: Don't negotiate deal terms yourself; get a lawyer
6. **Set expectations**: Clear timeline + communication cadence avoids surprises
7. **Celebrate wins**: If you get a GO decision, that's a major achievement! 🎉

---

## Sample Timeline: Start to Finish

```
Feb 26: Initial buyer contact (send EXECUTIVE_SUMMARY)
Feb 28: Buyer confirms interest → Send NDA
Mar 5: NDA signed ✅ → Send diligence package
Mar 6: Technical kickoff call
Mar 10-13: Buyer evaluates (internal technical review)
Mar 15: Weekly sync call #1
Mar 20-22: Buyer's architecture team completes assessment
Mar 25: Weekly sync call #2
Apr 1: Buyer confirms GO 🎉
Apr 2: Executive call → discuss terms
Apr 5: Buyer's legal sends LOI
Apr 10: Negotiate & counter-offer terms
Apr 15: Term sheet finalized
Apr 20-May 5: Full due diligence by buyer
May 10: Final definitive agreement signed ✅
May 15-30: IP transfer, final technical handoff
Jun 1: Closing & payment received 💰

Total time: ~95 days (roughly 3 months)
```

---

## Next Steps for You

1. **Customize NDA_TEMPLATE.md** with your company details
2. **Convert to PDF** and test readability
3. **Draft cover email** using the sample above
4. **Create shared folder** for documents (Google Drive, OneDrive, or private repo)
5. **Identify 3-5 target buyers** and start outreach
6. **Brief your legal counsel** on the deal structure you prefer

---

**Good luck with your sale! 🚀**
