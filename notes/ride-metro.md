# Ride Metro Protobuf

View compiled .rs file like this
```
cat $(find target/debug/build -name "*.rs" -path "*/api-*" | head -1)
```
Saved an example  TripUpdate at `notes/ride-metro-entity.txt`.

`FeedMessage`
  - `FeedHeader`
  - `FeedEntity` (array/repeated)
  - `TripUpdate` 
    - `StopTimeUpdate` (array,repated)
      - stop_sequence (xref `stop_times.txt` static gtfs data)
      - stop_id (xref `stops.txt`)
      - arrival `StopTimeEvent`
        - delay (relative to schedule)
        - time (absolute)
        - uncertainty
      - departure `StopTimeEvent`
  - `VehiclePosition`
  - `Alert`
  - etc


Each FeedEntity(TripUpdate) corresponds to a single vehicle on a route.

Rough flow:

Geolocation -> stop IDs -> routes at stop -> estimated time for nearest vehicle for each
direction on each route.

This will require a combination of static and realtime gtfs data.

Complexity should be handled on backend for memory efficiency, caching, etc.
