# EMCICalc 1.0.0 — Statistics conventions and limits

The Data / Statistics workbench resolves finite, dimensionless real cell expressions on the dedicated engine worker, then applies a bounded double-precision numerical kernel. Raising general calculator precision does not make this kernel arbitrary precision. Up to 10,000 grid rows are accepted; each frequency is a nonnegative integer at most 10,000,000 and total frequency N must be 1…10,000,000. A blank frequency means one. Required X/Y values must be present even when an existing row's frequency is zero.

## Frequency and summary conventions

Integer frequency means repeated observations for calculations, not arbitrary reliability weights. Zero-frequency rows remain in the grid, drafts, named data and CSV, but do not enter summary/fit/plot vectors. Each positive-frequency row appears once in scatter/residual plots; points are not expanded N times.

Sample variance and covariance divide by N−1; population versions divide by N. Sample statistics require N≥2. Standard error is the selected standard deviation divided by sqrt(N). Quantiles use linear interpolation at zero-based position (N−1)p in the frequency-expanded order (type 7), without physically expanding that data. Median, Q1, Q3 and IQR follow that rule. Every tied maximum-frequency value is listed as a mode. Geometric/harmonic means use a strictly positive X convention.

Population skewness is the standardized third central moment; excess kurtosis subtracts three from the standardized fourth moment. Sample skewness uses the adjusted factor sqrt(N(N−1))/(N−2), requiring N>2. Sample excess kurtosis uses (N−1)/((N−2)(N−3)) × ((N+1)g2+6), requiring N>3. Constant data has undefined standardized skew/kurtosis. Paired Pearson correlation is undefined if either variable is constant.

## Fits

| Model | Fitted relationship | Conditions |
| --- | --- | --- |
| Linear / quadratic / cubic | Polynomial degree 1 / 2 / 3 | Paired data and enough independent X information |
| Polynomial | Degree 1…8 | N≥degree+1 and full numerical rank |
| Exponential | log(Y) linear in X | Positive Y |
| Logarithmic | Y linear in log(X) | Positive X |
| Power | log(Y) linear in log(X) | Positive X and Y |

The kernel uses a frequency-weighted, centered/scaled Vandermonde matrix with twice-reorthogonalized QR, not normal equations. Its relative QR rank threshold is 10^−11. There are p=degree+1 fitted coefficients; transformed models have p=2. Identical X, insufficient observations, invalid transformed domains and nonfinite coefficients are rejected. Predictions must satisfy their own finite-range guards; an unavailable optional prediction does not discard an otherwise valid fit.

Exponential and power fits minimize residuals in log(Y), not original Y. Their reported original-Y metrics are descriptive and no retransformation bias correction is applied. The canonical equation preserves the center, scale and coefficients at 17 significant digits for exact double round-trip, rather than expanding a numerically fragile high-offset polynomial. Display summaries may use shorter numbers.

Let SSE be the frequency-weighted sum of squared original-Y residuals. Mean squared prediction error is SSE/N; its square root is RMSE. Residual MSE is SSE/(N−p), available only for positive residual degrees of freedom; residual standard error is its square root. R² is 1−SSE/SST when Y varies. Adjusted R² is 1−(1−R²)(N−1)/(N−p), requiring positive residual degrees of freedom.

AIC/BIC use an iid Gaussian original-Y likelihood and count variance as an additional fitted parameter: k=p+1. They are omitted for log-response models, exact/near-exact fits and zero residual degrees of freedom. They must not be compared as if an unavailable or differently defined likelihood were valid. Prediction is optional for a fit; Summary ignores a hidden Prediction draft.

## Storage, input and range

CSV fields have the same 4,096-character decoded limit whether quoted or unquoted. Invalid/oversized CSV is rejected transactionally before replacing the grid. Canonical means and fit equations use `max_digits10`-equivalent 17-digit serialization. Computing first commits the active cell editor; request generations and cancellation prevent stale output replacing a newer grid.

Scaled dimensionless correlation and standardized-moment formulas avoid intermediate denominator overflow/underflow where possible. Double intermediate underflow can occur well before the input approaches the smallest representable double. Summary rejects a nonzero X square lost to zero, lost X or paired-Y variation, and underflow after frequency normalization. Fits reject a nonzero residual square lost to zero or SSE/N underflow. Exponential/power predictions whose exponential underflows to zero are unavailable; an optional prediction failure preserves an otherwise valid fit. Actual zero and constant data remain valid for statistics that are defined for them.

Finite-range guards also remain for raw accumulated moments, regression transforms and residuals. Large-offset data still needs representable distinctions; no algorithm recovers information lost when values round to the same double. Rescale data when the tool reports a magnitude/rank limit. Numerical curves and residuals are analysis aids, not proof of model validity.
