# Historical metric coverage

Subscriber and view history is stored as UTC-day snapshots. The device calculates
changes from the current channel total against a stored baseline.

For a requested period (1, 2, 3, 7, 14 or 30 days), the preferred baseline is
the latest snapshot on or before the requested start day. If no such baseline
exists, the earliest available snapshot is used and the screen explicitly says
`Partial history: Nd of Md`. This exposes known changes without pretending that
missing earlier history was observed. It does not backfill or estimate missing
snapshots, and it cannot reconstruct changes before the first recorded snapshot.

This adjustment applies to both subscribers and views. YouTube public subscriber
counts may be rounded, so reported changes reflect the available public totals.
