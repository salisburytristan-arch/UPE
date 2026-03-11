#include "upe/upe_science_topics.h"

static int UpeScience_IsAsciiUpper(char c) {
    return (c >= 'A' && c <= 'Z') ? 1 : 0;
}

static char UpeScience_ToLowerAscii(char c) {
    if (UpeScience_IsAsciiUpper(c)) {
        return (char)(c - 'A' + 'a');
    }
    return c;
}

static int UpeScience_StringContainsIgnoreCase(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        return 0;
    }
    if (needle[0] == '\0') {
        return 1;
    }

    for (const char *h = haystack; *h != '\0'; ++h) {
        const char *h_cur = h;
        const char *n_cur = needle;
        while (*h_cur != '\0' && *n_cur != '\0' &&
               UpeScience_ToLowerAscii(*h_cur) == UpeScience_ToLowerAscii(*n_cur)) {
            ++h_cur;
            ++n_cur;
        }
        if (*n_cur == '\0') {
            return 1;
        }
    }

    return 0;
}

static const UpeScienceTopicStatus kScienceTopics[] = {
    {
        1u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Arithmetic > fractions and ratios (Example: 3/4 + 1/8 = 7/8)"
    },
    {
        2u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Prealgebra > exponents and roots (Example: 2^5 = 32 and sqrt(81)=9)"
    },
    {
        3u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Algebra I > linear equations (Example: 3x+2=11 -> x=3)"
    },
    {
        4u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Algebra I > inequalities (Example: 2x-1>7 -> x>4)"
    },
    {
        5u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Algebra II > polynomials (Example: x^2-5x+6=(x-2)(x-3))"
    },
    {
        6u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Algebra II > logarithms (Example: log10(1000)=3)"
    },
    {
        7u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Algebra II > complex numbers (Example: i^2=-1 and (1+i)^2=2i)"
    },
    {
        8u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Functions > composition (Example: f(x)=x^2 and g(x)=x+1 then f(g(2))=9)"
    },
    {
        9u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Trigonometry > identities (Example: sin^2(x)+cos^2(x)=1)"
    },
    {
        10u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Trigonometry > solving triangles (Example: Law of Cosines c^2=a^2+b^2-2ab cos(C))"
    },
    {
        11u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Analytic geometry > conics (Example: x^2/9 + y^2/4 = 1 ellipse)"
    },
    {
        12u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Vectors > dot product (Example: (1 2)·(3 4)=11)"
    },
    {
        13u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Vectors > cross product (Example: (1 0 0)x(0 1 0)=(0 0 1))"
    },
    {
        14u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Linear algebra > matrices (Example: solve Ax=b via Gaussian elimination)"
    },
    {
        15u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Linear algebra > eigenvalues (Example: A v = λ v)"
    },
    {
        16u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Calculus I > limits (Example: lim x->0 sin(x)/x = 1)"
    },
    {
        17u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Calculus I > derivatives (Example: d/dx x^3 = 3x^2)"
    },
    {
        18u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Calculus I > optimization (Example: maximize area A(x)=x(10-x))"
    },
    {
        19u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Calculus II > integration (Example: ∫ x^2 dx = x^3/3 + C)"
    },
    {
        20u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Calculus II > series (Example: 1/(1-x)=Σ x^n for |x|<1)"
    },
    {
        21u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Multivariable calculus > gradients (Example: ∇(x^2+y^2)=(2x 2y))"
    },
    {
        22u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Multivariable calculus > Lagrange multipliers (Example: maximize f subject to g=c via ∇f=λ∇g)"
    },
    {
        23u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Differential equations > first order ODEs (Example: dy/dx = ky -> y=Ce^(kx))"
    },
    {
        24u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Differential equations > oscillators (Example: x¨+ω^2 x=0)"
    },
    {
        25u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Dynamical systems > phase space (Example: logistic map x_{n+1}=r x_n(1-x_n))"
    },
    {
        26u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Numerical analysis > floating point error (Example: subtractive cancellation when a≈b in a-b)"
    },
    {
        27u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Numerical analysis > stable integration (Example: RK4 vs Euler for stiff systems)"
    },
    {
        28u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Discrete math > sets and relations (Example: equivalence relation partitions a set)"
    },
    {
        29u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Discrete math > logic and proofs (Example: prove by contradiction assume ¬P then derive false)"
    },
    {
        30u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Combinatorics > counting (Example: n choose k = n!/(k!(n-k)!))"
    },
    {
        31u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Combinatorics > inclusion exclusion (Example: |A∪B|=|A|+|B|-|A∩B|)"
    },
    {
        32u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Graph theory > shortest paths (Example: Dijkstra on nonnegative edges)"
    },
    {
        33u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Graph theory > spanning trees (Example: Kruskal for MST)"
    },
    {
        34u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Probability > random variables (Example: E[X]=Σ x p(x))"
    },
    {
        35u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Probability > Bayes rule (Example: P(A|B)=P(B|A)P(A)/P(B))"
    },
    {
        36u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Statistics > estimation (Example: sample mean estimates μ)"
    },
    {
        37u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Statistics > hypothesis tests (Example: t test compares mean to baseline)"
    },
    {
        38u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Optimization > convex optimization (Example: minimize ||Ax-b||^2)"
    },
    {
        39u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Optimization > constrained optimization (Example: KKT conditions)"
    },
    {
        40u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Information theory > entropy (Example: H(X)=-Σ p log2 p)"
    },
    {
        41u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Game theory > Nash equilibrium (Example: best responses intersect)"
    },
    {
        42u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Set theory > cardinality (Example: countable vs uncountable)"
    },
    {
        43u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Real analysis > continuity (Example: ε-δ definition)"
    },
    {
        44u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Real analysis > measure theory (Example: Lebesgue integral handles more functions than Riemann)"
    },
    {
        45u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Functional analysis > norms (Example: ||x||2 = sqrt(Σ x_i^2))"
    },
    {
        46u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Abstract algebra > groups (Example: integers under addition form a group)"
    },
    {
        47u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Abstract algebra > rings (Example: polynomials with addition and multiplication)"
    },
    {
        48u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Abstract algebra > fields (Example: rationals form a field)"
    },
    {
        49u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Topology > open sets (Example: ball B(r) around a point)"
    },
    {
        50u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Differential geometry > manifolds (Example: sphere S2 is a 2D manifold)"
    },
    {
        51u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Category theory > functors (Example: map objects and morphisms preserving composition)"
    },
    {
        52u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Atomic structure (Example: electron configuration 1s2 2s2 2p6 for Ne)"
    },
    {
        53u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Periodic trends (Example: electronegativity increases across a period)"
    },
    {
        54u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Chemical bonding > ionic (Example: Na+ and Cl- form NaCl)"
    },
    {
        55u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Chemical bonding > covalent (Example: H2 shares electron pair)"
    },
    {
        56u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Molecular geometry > VSEPR (Example: CH4 tetrahedral)"
    },
    {
        57u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Intermolecular forces (Example: H bonding raises boiling point of water)"
    },
    {
        58u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Stoichiometry (Example: 2H2 + O2 -> 2H2O so 2 mol H2 makes 2 mol H2O)"
    },
    {
        59u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Gas laws (Example: PV=nRT)"
    },
    {
        60u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Solutions and concentration (Example: molarity M = mol solute / L solution)"
    },
    {
        61u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Thermochemistry (Example: ΔH<0 exothermic)"
    },
    {
        62u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Chemical kinetics (Example: rate=k[A]^m[B]^n)"
    },
    {
        63u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Reaction mechanisms (Example: SN2 is one step backside attack)"
    },
    {
        64u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Chemical equilibrium (Example: K = products/reactants at equilibrium)"
    },
    {
        65u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Le Chatelier principle (Example: add reactant shifts equilibrium to products)"
    },
    {
        66u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Acids and bases (Example: pH = -log10[H+])"
    },
    {
        67u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Buffers (Example: weak acid + conjugate base resists pH change)"
    },
    {
        68u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Solubility equilibria (Example: Ksp for AgCl relates to [Ag+][Cl-])"
    },
    {
        69u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Redox chemistry (Example: oxidation is loss of electrons)"
    },
    {
        70u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Electrochemistry > galvanic cells (Example: cell potential drives spontaneous reaction)"
    },
    {
        71u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Electrochemistry > Nernst equation (Example: E=E0-(RT/nF)lnQ)"
    },
    {
        72u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Organic chemistry > functional groups (Example: alcohol ROH)"
    },
    {
        73u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Organic chemistry > stereochemistry (Example: chirality gives R and S enantiomers)"
    },
    {
        74u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Organic chemistry > aromaticity (Example: benzene has 6 π electrons)"
    },
    {
        75u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Organic chemistry > spectroscopy (Example: IR peak near 1700 cm^-1 indicates C=O)"
    },
    {
        76u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Inorganic chemistry > coordination complexes (Example: [Cu(NH3)4]2+)"
    },
    {
        77u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Solid state chemistry (Example: band gap separates valence and conduction)"
    },
    {
        78u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Polymer chemistry (Example: polyethylene from ethylene monomers)"
    },
    {
        79u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Analytical chemistry > titration (Example: find unknown acid concentration with base)"
    },
    {
        80u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Analytical chemistry > chromatography (Example: separate mixture by retention time)"
    },
    {
        81u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Biochemistry > enzymes (Example: Michaelis Menten v=Vmax[S]/(Km+[S]))"
    },
    {
        82u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Materials chemistry (Example: Li ion cathode intercalation)"
    },
    {
        83u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Environmental chemistry (Example: NOx contributes to ozone formation)"
    },
    {
        84u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Natal chart basics (Example: Sun sign = zodiac sign at birth)"
    },
    {
        85u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Zodiac signs archetypes (Example: Sun in Aries interpreted as assertive style)"
    },
    {
        86u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Houses (Example: 10th house linked with career themes)"
    },
    {
        87u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Aspects (Example: conjunction = 0 degree separation)"
    },
    {
        88u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Transits (Example: Saturn transit to natal Moon interpreted as heavier mood)"
    },
    {
        89u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Progressions (Example: progressed Moon changes sign roughly monthly)"
    },
    {
        90u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Synastry (Example: compare two charts for relationship themes)"
    },
    {
        91u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Mercury retrograde lore (Example: people attribute delays to Mercury retrograde)"
    },
    {
        92u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Celestial sphere coordinates (Example: right ascension and declination locate stars)"
    },
    {
        93u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Timekeeping > sidereal vs solar (Example: sidereal day is based on stars)"
    },
    {
        94u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Telescopes > optics (Example: angular resolution ~ λ/D)"
    },
    {
        95u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Photometry (Example: magnitude scale is logarithmic)"
    },
    {
        96u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Spectroscopy (Example: absorption lines identify elements)"
    },
    {
        97u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Stellar structure (Example: hydrostatic equilibrium balances gravity and pressure)"
    },
    {
        98u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Stellar evolution (Example: main sequence -> red giant -> white dwarf)"
    },
    {
        99u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "HR diagram (Example: luminosity vs temperature tracks stellar life stages)"
    },
    {
        100u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Nucleosynthesis (Example: fusion H->He releases energy)"
    },
    {
        101u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Supernovae (Example: Type Ia used as standardizable candles)"
    },
    {
        102u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Neutron stars (Example: pulsar lighthouse effect)"
    },
    {
        103u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Black holes (Example: event horizon radius r_s=2GM/c^2)"
    },
    {
        104u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Accretion disks (Example: disk heats by viscosity and radiates X rays)"
    },
    {
        105u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Exoplanets > detection (Example: transit method measures dip in brightness)"
    },
    {
        106u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Exoplanets > radial velocity (Example: Doppler wobble reveals planet mass lower bound)"
    },
    {
        107u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Planet formation (Example: core accretion builds gas giants)"
    },
    {
        108u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Protoplanetary disks (Example: ALMA images dust rings)"
    },
    {
        109u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Solar system dynamics (Example: resonances like 3:2 Pluto Neptune)"
    },
    {
        110u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Celestial mechanics (Example: Kepler third law P^2 ∝ a^3)"
    },
    {
        111u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Orbital perturbations (Example: J2 oblateness shifts node)"
    },
    {
        112u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Tides (Example: tidal locking of Moon)"
    },
    {
        113u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Galactic structure (Example: Milky Way has spiral arms)"
    },
    {
        114u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Stellar populations (Example: Population II stars are metal poor)"
    },
    {
        115u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Interstellar medium (Example: molecular clouds form stars)"
    },
    {
        116u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Star formation (Example: Jeans instability triggers collapse)"
    },
    {
        117u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Dark matter evidence (Example: flat galaxy rotation curves)"
    },
    {
        118u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Dark energy and expansion (Example: accelerated expansion from supernova distances)"
    },
    {
        119u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Cosmology > Big Bang (Example: CMB is relic radiation)"
    },
    {
        120u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Cosmology > Friedmann equations (Example: expansion rate depends on density terms)"
    },
    {
        121u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Gravitational lensing (Example: mass bends light and magnifies background galaxies)"
    },
    {
        122u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Gravitational waves (Example: binary merger chirp signals)"
    },
    {
        123u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Astrobiology (Example: habitable zone concept)"
    },
    {
        124u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Classical mechanics > Newton laws (Example: F=ma)"
    },
    {
        125u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Classical mechanics > energy (Example: E=K+U conserved in closed system)"
    },
    {
        126u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Classical mechanics > momentum (Example: p=mv conserved in isolated collisions)"
    },
    {
        127u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Classical mechanics > rigid body dynamics (Example: τ=Iα)"
    },
    {
        128u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Classical mechanics > Lagrangian mechanics (Example: d/dt(∂L/∂qdot)-∂L/∂q=0)"
    },
    {
        129u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Classical mechanics > Hamiltonian mechanics (Example: qdot=∂H/∂p and pdot=-∂H/∂q)"
    },
    {
        130u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Waves and oscillations (Example: x¨+2ζωx˙+ω^2 x=0)"
    },
    {
        131u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Fluid dynamics > Navier Stokes (Example: model viscous flow with ∂v/∂t + v·∇v = -∇p/ρ + ν∇^2 v)"
    },
    {
        132u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Fluid dynamics > turbulence (Example: Reynolds number Re=ρvL/μ)"
    },
    {
        133u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Thermodynamics > laws (Example: ΔU=Q-W)"
    },
    {
        134u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Statistical mechanics (Example: Boltzmann factor exp(-E/kT))"
    },
    {
        135u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Electromagnetism > Coulomb law (Example: F = k q1 q2 / r^2)"
    },
    {
        136u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Electromagnetism > Maxwell equations (Example: ∇·E=ρ/ε0)"
    },
    {
        137u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Circuits (Example: V=IR)"
    },
    {
        138u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Optics > geometrical (Example: thin lens 1/f=1/do+1/di)"
    },
    {
        139u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Optics > wave optics (Example: double slit interference fringe spacing ~ λL/d)"
    },
    {
        140u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Quantum mechanics > wavefunction (Example: |ψ|^2 gives probability density)"
    },
    {
        141u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Quantum mechanics > Schrodinger equation (Example: iħ ∂ψ/∂t = Hψ)"
    },
    {
        142u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Quantum mechanics > operators (Example: [x p]=iħ)"
    },
    {
        143u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Quantum mechanics > tunneling (Example: alpha decay via barrier penetration)"
    },
    {
        144u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Quantum field theory basics (Example: particles as field excitations)"
    },
    {
        145u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Relativity > special relativity (Example: time dilation γ=1/sqrt(1-v^2/c^2))"
    },
    {
        146u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Relativity > general relativity (Example: gravity as spacetime curvature)"
    },
    {
        147u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Nuclear physics (Example: binding energy from mass defect)"
    },
    {
        148u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Particle physics > Standard Model (Example: quarks and leptons interact via gauge bosons)"
    },
    {
        149u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Condensed matter > band theory (Example: conductor vs insulator from band gap)"
    },
    {
        150u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Condensed matter > superconductivity (Example: zero resistance below Tc)"
    },
    {
        151u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Materials physics > elasticity (Example: stress = E strain)"
    },
    {
        152u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Plasma physics (Example: Debye length sets shielding scale)"
    },
    {
        153u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Acoustics (Example: sound speed c = sqrt(γRT/M) in ideal gas)"
    },
    {
        154u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Computational physics (Example: simulate N body with symplectic integrator)"
    },
    {
        155u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Chaos (Example: sensitive dependence on initial conditions)"
    },
    {
        156u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Geophysics (Example: seismic waves probe Earth interior)"
    },
    {
        157u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Biophysics (Example: diffusion J=-D∇c)"
    },
    {
        158u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Bosonic string basics (Example: vibrating string modes map to particle states)"
    },
    {
        159u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Superstrings and supersymmetry (Example: fermions emerge via SUSY on worldsheet)"
    },
    {
        160u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Worldsheet action (Example: Polyakov action defines string dynamics)"
    },
    {
        161u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Conformal field theory (Example: 2D CFT describes string worldsheet)"
    },
    {
        162u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Compactification (Example: extra dimensions wrapped on small manifold)"
    },
    {
        163u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Calabi Yau manifolds (Example: compact spaces preserving SUSY)"
    },
    {
        164u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "D branes (Example: open strings end on branes)"
    },
    {
        165u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Gauge gravity duality AdS CFT (Example: gravity in AdS corresponds to CFT on boundary)"
    },
    {
        166u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "String scattering amplitudes (Example: Veneziano amplitude in early models)"
    },
    {
        167u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Moduli stabilization (Example: fix shape and size of extra dimensions)"
    },
    {
        168u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "M theory (Example: unifies superstring theories in 11D)"
    },
    {
        169u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Flux compactifications (Example: background fluxes generate potentials)"
    },
    {
        170u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "String cosmology (Example: brane inflation scenarios)"
    },
    {
        171u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Heliophysics (Example: track solar cycle impact on Earth)"
    },
    {
        172u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Solar wind (Example: plasma outflow interacts with magnetosphere)"
    },
    {
        173u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Space weather forecasting (Example: CME arrival predicts geomagnetic storms)"
    },
    {
        174u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Magnetospheres (Example: Earth magnetosphere deflects charged particles)"
    },
    {
        175u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Ionosphere physics (Example: electron density affects radio propagation)"
    },
    {
        176u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Auroras (Example: particle precipitation excites atmospheric atoms)"
    },
    {
        177u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Radiation belts (Example: Van Allen belts trap energetic electrons)"
    },
    {
        178u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Cosmic rays (Example: high energy protons produce air showers)"
    },
    {
        179u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Solar energetic particles (Example: flare events increase radiation risk)"
    },
    {
        180u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Plasma waves (Example: whistler mode waves scatter electrons)"
    },
    {
        181u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Reconnection (Example: magnetic reconnection releases stored magnetic energy)"
    },
    {
        182u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Planetary atmospheres (Example: escape processes strip upper atmosphere)"
    },
    {
        183u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Atmospheric drag (Example: LEO satellites decay due to drag)"
    },
    {
        184u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Orbital mechanics > Hohmann transfer (Example: two burn transfer between circular orbits)"
    },
    {
        185u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Attitude dynamics (Example: reaction wheels change spacecraft orientation)"
    },
    {
        186u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Spacecraft thermal control (Example: radiators reject heat by IR emission)"
    },
    {
        187u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Astrodynamics > perturbations (Example: solar radiation pressure changes orbit)"
    },
    {
        188u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Space instrumentation (Example: magnetometer measures field vectors)"
    },
    {
        189u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Remote sensing (Example: multispectral imaging retrieves surface properties)"
    },
    {
        190u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Planetary magnetism (Example: Jupiter strong field drives intense auroras)"
    },
    {
        191u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "CPU profiling first (Example: use perf or VTune to find hottest function)"
    },
    {
        192u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Choose better algorithms (Example: O(n log n) sort vs O(n^2) bubble)"
    },
    {
        193u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Data locality and cache (Example: iterate arrays in contiguous order)"
    },
    {
        194u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Structure of arrays over array of structures (Example: positions_x positions_y for tight loops)"
    },
    {
        195u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Minimize allocations (Example: reuse buffers via memory pool)"
    },
    {
        196u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Avoid fragmentation (Example: arena allocator for many short lived objects)"
    },
    {
        197u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Reduce branch mispredicts (Example: replace if chains with lookup table)"
    },
    {
        198u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "SIMD vectorization (Example: process 8 floats at once with AVX)"
    },
    {
        199u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Align data (Example: 64 byte alignment for cache lines)"
    },
    {
        200u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use restrict and const when safe (Example: help compiler auto vectorize)"
    },
    {
        201u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Loop optimizations (Example: unroll small fixed loops)"
    },
    {
        202u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Avoid false sharing in threads (Example: pad hot counters to cache line size)"
    },
    {
        203u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Threading with work stealing (Example: task queue balances uneven jobs)"
    },
    {
        204u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Pin threads to cores (Example: keep worker on same core to preserve cache)"
    },
    {
        205u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Reduce lock contention (Example: per thread buffers then merge)"
    },
    {
        206u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use atomics carefully (Example: relaxed ordering for counters)"
    },
    {
        207u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Batch system calls (Example: write buffers instead of many small writes)"
    },
    {
        208u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "I O overlap (Example: async read while computing)"
    },
    {
        209u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Compiler flags (Example: -O3 and LTO for release builds)"
    },
    {
        210u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Profile guided optimization (Example: compile with PGO using real workloads)"
    },
    {
        211u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Avoid denormals in hot DSP loops (Example: flush to zero mode)"
    },
    {
        212u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use fixed point where needed (Example: embedded deterministic math)"
    },
    {
        213u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "GPU occupancy management (Example: reduce register pressure to increase active warps)"
    },
    {
        214u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Coalesced global memory access (Example: threads read consecutive addresses)"
    },
    {
        215u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use shared memory tiling (Example: tile matrix multiply into shared then compute)"
    },
    {
        216u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Minimize global memory writes (Example: fuse kernels to keep intermediates in registers)"
    },
    {
        217u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Avoid warp divergence (Example: replace per thread branching with predication when possible)"
    },
    {
        218u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use appropriate precision (Example: FP16 for inference when error tolerable)"
    },
    {
        219u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use vector loads (Example: load float4 per thread when aligned)"
    },
    {
        220u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use constant memory for broadcast (Example: store small read only tables)"
    },
    {
        221u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use texture cache for spatial locality (Example: sampling 2D fields)"
    },
    {
        222u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Overlap compute and transfer (Example: double buffer with streams)"
    },
    {
        223u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Reduce PCIe transfers (Example: keep data on GPU across steps)"
    },
    {
        224u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Kernel launch overhead reduction (Example: larger kernels or persistent kernels)"
    },
    {
        225u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use graph execution when available (Example: capture repeated launches as a graph)"
    },
    {
        226u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Memory pooling on GPU (Example: reuse allocations to avoid cudaMalloc overhead)"
    },
    {
        227u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Avoid CPU GPU sync points (Example: do not call blocking device synchronize in loop)"
    },
    {
        228u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use pinned host memory for transfers (Example: faster H2D copies)"
    },
    {
        229u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use unified memory carefully (Example: prefetch to device to avoid page faults)"
    },
    {
        230u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "RAM usage profiling (Example: track peak RSS and per allocation callsites)"
    },
    {
        231u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Reduce object size (Example: pack booleans into bitsets)"
    },
    {
        232u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Compress sparse data (Example: CSR format for sparse matrices)"
    },
    {
        233u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use streaming and chunking (Example: process dataset in tiles not full load)"
    },
    {
        234u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Cache results (Example: memoize expensive pure function)"
    },
    {
        235u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Avoid copies by move semantics (Example: C++ move vector instead of deep copy)"
    },
    {
        236u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use zero copy parsing (Example: parse views into buffer slices)"
    },
    {
        237u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Prefer iterative over recursive when deep (Example: avoid stack blowups)"
    },
    {
        238u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Control GC pressure in managed runtimes (Example: reuse arrays and avoid per frame allocations)"
    },
    {
        239u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "NUMA awareness (Example: allocate memory on same NUMA node as thread)"
    },
    {
        240u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Huge pages when beneficial (Example: reduce TLB misses for large contiguous buffers)"
    },
    {
        241u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Memory bandwidth optimization (Example: compute intensity increase via fusion)"
    },
    {
        242u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Precision tuned math libraries (Example: use fused multiply add where available)"
    },
    {
        243u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Deterministic scheduling for sims (Example: fixed timestep and deterministic reduction order)"
    },
    {
        244u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Deterministic parallel reductions (Example: pairwise reduction tree to fix summation order)"
    },
    {
        245u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use SoA for GPU and CPU parity (Example: same layout friendly to vector loads)"
    },
    {
        246u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Fast path slow path design (Example: common case branchless and rare case separate)"
    },
    {
        247u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Minimize virtual dispatch (Example: replace hot virtual calls with function pointers or templates)"
    },
    {
        248u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Precompute lookup tables (Example: approximate sin with table and interpolation)"
    },
    {
        249u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Bounds check elimination (Example: hoist checks out of inner loops)"
    },
    {
        250u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Use cache friendly spatial partitioning (Example: uniform grid broadphase reduces pair checks)"
    },
    {
        251u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Numerical PDEs > finite volume methods (Example: conservative flux update for hyperbolic systems)"
    },
    {
        252u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Stochastic calculus > Ito processes (Example: dX = mu dt + sigma dW for diffusion models)"
    },
    {
        253u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Spectral methods > Fourier collocation (Example: solve periodic PDEs with FFT basis)"
    },
    {
        254u,
        UPE_SCIENCE_DOMAIN_MATH,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Computational geometry > convex hulls (Example: quickhull for collision pre-processing)"
    },
    {
        255u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Quantum chemistry > Hartree-Fock (Example: self-consistent field for molecular orbitals)"
    },
    {
        256u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Chemical thermodynamics > Gibbs free energy (Example: Delta G determines reaction spontaneity)"
    },
    {
        257u,
        UPE_SCIENCE_DOMAIN_CHEM,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Atmospheric chemistry > photochemical smog (Example: VOC and NOx driven ozone cycles)"
    },
    {
        258u,
        UPE_SCIENCE_DOMAIN_ASTROLOGY,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Statistical validation of astrological claims (Example: blinded study design for predictive tests)"
    },
    {
        259u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "High-energy astrophysics > AGN jets (Example: relativistic outflows from accreting black holes)"
    },
    {
        260u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Interferometry > aperture synthesis (Example: combine baselines for high angular resolution)"
    },
    {
        261u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Transient astronomy > kilonova modeling (Example: r-process ejecta light curve estimation)"
    },
    {
        262u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Black hole metrics > Kerr spacetime (Example: frame dragging near rotating horizons)"
    },
    {
        263u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Computational mechanics > symplectic integrators (Example: long-horizon Hamiltonian stability)"
    },
    {
        264u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Nonlinear dynamics > bifurcation analysis (Example: Hopf bifurcation in oscillator systems)"
    },
    {
        265u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Continuum mechanics > constitutive models (Example: viscoelastic stress evolution laws)"
    },
    {
        266u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Plasma transport > magnetohydrodynamics (Example: coupling Lorentz forces with fluid flow)"
    },
    {
        267u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "String compactification > moduli spaces (Example: deformation parameters of compact manifolds)"
    },
    {
        268u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Brane dynamics > Dirac-Born-Infeld action (Example: effective worldvolume dynamics)"
    },
    {
        269u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Dualities > T-duality and S-duality (Example: equivalent spectra across coupling regimes)"
    },
    {
        270u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Holography > AdS black brane thermodynamics (Example: transport coefficients from dual gravity)"
    },
    {
        271u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Mission design > patched conics (Example: sphere-of-influence transfer staging)"
    },
    {
        272u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Orbit determination > batch least squares (Example: fit state vector to tracking residuals)"
    },
    {
        273u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Attitude control > quaternion PID (Example: reaction wheel torque commands from error quaternion)"
    },
    {
        274u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Space environment > thermospheric density models (Example: drag forecasting under solar storms)"
    },
    {
        275u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Numerical kernels > blocked sparse matvec (Example: cache-aware CSR traversal with tiling)"
    },
    {
        276u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Deterministic replay > checksum windows (Example: periodic state digests for bisecting drift)"
    },
    {
        277u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Parallel runtime > lock-free MPMC queues (Example: bounded ring queue for task dispatch)"
    },
    {
        278u,
        UPE_SCIENCE_DOMAIN_OPT,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Profiling automation > continuous flamegraph diffs (Example: CI gate on hot-path regression)"
    },
    {
        279u,
        UPE_SCIENCE_DOMAIN_PHYSICS,
        UPE_SCIENCE_LEVEL_EXTREME,
        "Geometric mechanics > Lie group integration (Example: SO(3) updates without renormalization drift)"
    },
    {
        280u,
        UPE_SCIENCE_DOMAIN_SPACE,
        UPE_SCIENCE_LEVEL_MINIMAL,
        "Deep-space navigation > DSN two-way ranging (Example: light-time corrected range and Doppler solution)"
    },
    {
        281u,
        UPE_SCIENCE_DOMAIN_ASTRONOMY,
        UPE_SCIENCE_LEVEL_JUST_STARTED,
        "Cosmological probes > baryon acoustic oscillations (Example: standard ruler in galaxy clustering)"
    },
    {
        282u,
        UPE_SCIENCE_DOMAIN_STRING,
        UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED,
        "Worldsheet quantization > BRST cohomology (Example: physical state conditions from nilpotent charge)"
    },
};

uint32_t UpeScience_GetTopicCount(void) {
    return (uint32_t)(sizeof(kScienceTopics) / sizeof(kScienceTopics[0]));
}

uint32_t UpeScience_GetTopics(UpeScienceTopicStatus *out_topics, uint32_t max_topics) {
    uint32_t count = UpeScience_GetTopicCount();
    if (!out_topics || max_topics == 0) {
        return count;
    }
    if (count > max_topics) {
        count = max_topics;
    }
    for (uint32_t i = 0; i < count; ++i) {
        out_topics[i] = kScienceTopics[i];
    }
    return count;
}

uint32_t UpeScience_GetTopicsByDomain(UpeScienceDomain domain, UpeScienceTopicStatus *out_topics, uint32_t max_topics) {
    uint32_t written = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].domain == domain) {
            if (out_topics && written < max_topics) {
                out_topics[written] = kScienceTopics[i];
            }
            written += 1u;
        }
    }
    return written;
}

uint32_t UpeScience_GetTopicsByLevel(UpeScienceImplementationLevel level, UpeScienceTopicStatus *out_topics, uint32_t max_topics) {
    uint32_t written = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].level == level) {
            if (out_topics && written < max_topics) {
                out_topics[written] = kScienceTopics[i];
            }
            written += 1u;
        }
    }
    return written;
}

uint32_t UpeScience_GetTopicsByDomainAndLevel(UpeScienceDomain domain,
                                              UpeScienceImplementationLevel level,
                                              UpeScienceTopicStatus *out_topics,
                                              uint32_t max_topics) {
    uint32_t written = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].domain == domain && kScienceTopics[i].level == level) {
            if (out_topics && written < max_topics) {
                out_topics[written] = kScienceTopics[i];
            }
            written += 1u;
        }
    }
    return written;
}

uint32_t UpeScience_FindTopicsContaining(const char *substring, UpeScienceTopicStatus *out_topics, uint32_t max_topics) {
    if (!substring) {
        return 0;
    }
    uint32_t written = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (UpeScience_StringContainsIgnoreCase(kScienceTopics[i].topic, substring)) {
            if (out_topics && written < max_topics) {
                out_topics[written] = kScienceTopics[i];
            }
            written += 1u;
        }
    }
    return written;
}

int UpeScience_GetTopicById(uint32_t id, UpeScienceTopicStatus *out_topic) {
    if (!out_topic || id == 0) {
        return 0;
    }
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].id == id) {
            *out_topic = kScienceTopics[i];
            return 1;
        }
    }
    return 0;
}

uint32_t UpeScience_GetDomainTopicCount(UpeScienceDomain domain) {
    uint32_t count = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].domain == domain) {
            count += 1u;
        }
    }
    return count;
}

uint32_t UpeScience_GetDomainImplementedTopicCount(UpeScienceDomain domain) {
    uint32_t count = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].domain == domain &&
            kScienceTopics[i].level != UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED) {
            count += 1u;
        }
    }
    return count;
}

uint32_t UpeScience_GetLevelTopicCount(UpeScienceImplementationLevel level) {
    uint32_t count = 0;
    uint32_t total = UpeScience_GetTopicCount();
    for (uint32_t i = 0; i < total; ++i) {
        if (kScienceTopics[i].level == level) {
            count += 1u;
        }
    }
    return count;
}
