# Matrix and Graph numerical/resource limits

## Matrix workbench

Entry remains 1…32 rows and 1…32 columns, with exact expressions in each cell. Dimension controls do not silently discard nonempty cells: clear the affected cells before reducing dimensions. Row/column swap, scale and add-scaled operations validate their actual fields only. Binary operations require the appropriate B grid; hidden B data does not invalidate a unary operation.

| Operation | Constraint or convention |
| --- | --- |
| Add/subtract/Hadamard | Equal dimensions |
| Matrix multiply | A column count equals B row count |
| Determinant/inverse/trace/LU/minors/cofactors/adjugate/characteristic/eigen | Square matrix required |
| Hermitian transpose | Transpose with conjugation of every cell |
| Minors/cofactors | Analysis limited to 8 × 8; entry remains 32 × 32 |
| Characteristic polynomial | At most 6 × 6; `det(xI−A)` convention |
| Kronecker | Output at most 32 × 32 |
| Vector norm/normalize/dot/cross | One row or column; matching lengths for dot; exactly three entries for cross |
| Normalize/inverse | Zero vector or singular matrix rejected |
| Echelon | The reduced row-echelon result is also an echelon form |
| Exact explanatory RREF | Steps supports at most 6 × 6 rational cells |
| Exact explanatory determinant/inverse | Steps supports 2 × 2 rational cells |

Numerical partial-pivot LU supports up to 32 × 32. It returns `[P,L,U]` with `P*A=L*U`, a relative maximum residual and a pivot tolerance of 10^−11 times the magnitude scale. A relative residual above 10^−8 or any nonfinite intermediate is rejected.

Numerical eigen analysis supports up to 8 × 8 using a shifted QR procedure bounded to 4,000 iterations. It reports eigenvalues, algebraic multiplicities and normalized eigenvectors; grouping/rank tolerance is 10^−7 times scale. Residuals are checked, and an incomplete eigenbasis is marked non-diagonalizable at that tolerance. Nonconvergence, singular intermediate basis and overflow are explained. These bounded double-complex decompositions do not promise arbitrary precision, exact multiplicities, or a proof of diagonalizability for nearly defective data. Canonical numeric entries retain 17 significant digits.

General matrix expression requests have a 250,000-character operation budget. All analysis runs on the existing worker; cancellation and generation checks reject stale UI output. Output inspection/Insert is separate from a normal Calculate commit.

## Graph and Table

The graph model supports up to 20 configured series. Types are explicit, polar, parametric, implicit equality, less-than-or-equal region, greater-than-or-equal region and z=f(x,y) surface. Scatter/fit/residual views for a dataset belong to Data / Statistics, not an invented eighth Graph series kind.

A graph request has a shared default limit of 24,000 scalar evaluations and 6,000 milliseconds for all enabled series, rather than a full independent timeout per series. Sampling quality selects bounded resolution/adaptation. Scalar calls, annotations, contours and table work honor cancellation; stale results are ignored. A slow backend call can consume time before returning, so the deadline is not a hard real-time scheduling promise.

Invalid/nonfinite samples are gaps. Pole and contour validation prevents known false line/fill cases; it does not prove absence of every discontinuity. Roots, intersections, extrema and inflection annotations are numerical candidates with sampled tolerances. Trapezoidal integral shading is available only on a sampled interval without gaps. Genuine 3D triangles retain double world coordinates and are projected interactively by the local renderer.

Tables accept up to 10,000 requested inputs with validated range/step. Parametric/polar tables use the parameter; implicit/surface tables use the documented y=0 slice. A timeout keeps requested input rows and their status; explicit cancellation may return early. CSV exports every returned row, including error status and series angle. Reset changes the view; Clear removes series. PNG exports the current 2D/3D view, SVG is a 2D export, and CSV is numerical data.
