# EMCICalc 1.0.0 Desktop — exact offline Steps coverage

Steps is a deterministic local rule engine (`src/stepsengine.cpp`), not a remote explanation service or a transcript guessed from a CAS answer. The general libqalculate engine and Steps have different capability and resource boundaries. An unsupported derivation does not invalidate an independently valid committed result.

| Operation | Supported family | Deliberate boundary |
| --- | --- | --- |
| Rational arithmetic | Exact fractions, rational expression arithmetic and reduction | Reduced numerator and denominator magnitudes at most 10^12 |
| Recurring decimal | `0.(3)`, `0.1(6)`, `12.34(56)` and signed variants | At most six digits in each whole/nonrecurring/repeating part |
| Simplify / expand | One-variable rational-polynomial expressions, collection and denominator cancellation with exclusions retained | Degree at most 8; integer rational powers −8…8 |
| Factor | Constant, affine and quadratic rational-coefficient polynomials | No general higher-degree factorization |
| Solve equation | Affine and quadratic equations, with exact rational or quadratic-radical roots | No general transcendental equation derivation |
| Absolute-value equation | `abs(a*x+b)=c*x+d`, rational coefficients | Branch conditions are checked; not arbitrary nested absolute values |
| Radical equation | `sqrt(a*x+b)=c*x+d` | Surviving candidate roots must be rational and satisfy the original unsquared equation |
| Inequality | One real variable; combined numerator and denominator each degree ≤2 | Denominator exclusions retained; very close distinct algebraic breakpoints are rejected when order cannot be certified |
| Derivative | Constants, selected variable, sum/difference, product, quotient, powers −8…8 and supported sin/cos/tan/exp/ln/sqrt chains | Preconditions and non-differentiable boundaries remain explicit; abs derivatives and general special-function rules are unsupported |
| Polynomial integral | Rational polynomials degree 0…7 | Adds an arbitrary constant; not a definite-integral convergence proof |
| Elementary integral | sin/cos/exp of a nonconstant affine argument; constant divided by an affine expression | Interval/domain restrictions retained for logarithmic primitive |
| Substitution | Matched inner derivative times a supported elementary outer expression, or an integer power −7…7 | Exact matching procedure, not general substitution search |
| Integration by parts | An affine factor times exp/sin/cos of an affine argument | Does not promise arbitrary repeated integration by parts |
| Limits | Rational functions at rational points and ±infinity; matched affine sine ratios; supported elementary interior continuity | Endpoint log/sqrt analysis and non-rational infinity limits may be unsupported |
| RREF / Gaussian elimination | Exact rational matrices with 1…6 rows and 1…6 columns | Bounded arithmetic and resource limits still apply |
| Determinant / inverse | Exact rational 2 × 2 matrices | Singular inverse rejected; larger matrices use other workbench paths without a promised derivation |

Global bounds: 1,024 input characters; 256 syntax nodes; nesting depth 32; 50,000 counted operations; 12 digits in a decimal literal. Integral recursive decomposition is limited to depth 12. These are implementation bounds, not precision settings.

Each step supplies before/after mathematical data, a rule/method identifier, an explanation, conditions and domain notes. Nested procedures remain expandable; text and canonical expressions are reusable. More actions provides Copy steps, Expand / collapse all, Export PDF and Export PNG through native Save dialogs; a derivation taller than the PNG size bound requires PDF. Excluded input points and rejected extraneous roots remain visible even when the simplified expression no longer shows the original denominator or radical.

Use the frozen expression of the selected result when inspecting a saved result. Definitions, Ans, M and R0–R9 must be resolved from that result context; inspecting Steps must not recalculate a random leaf or commit history. Cancelling a derivation must discard stale output. Configured automated coverage and platform limits are documented in [CI](../CI.md).

Relevant source and tests: `src/stepsengine.cpp`, `src/stepspanel.cpp`, `src/engine.cpp`, `tests/steps_tests.cpp`, the result-context integration fixtures and the CTest definitions. No claim is made for arbitrary CAS proof, every special-function derivation, OCR, camera recognition or examination mode.
