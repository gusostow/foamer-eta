https://api-doc.transitapp.com/

I emailed Transit and they removed the monthly limit from my API key (saved in .envrc.local).

This looks substantially easier to work with than ride-metro.
- Server-side filtering
- Tell you nearby stops directly
- JSON

Logic

- `/public/nearby_stops` Refresh nearby stops only when booted, or when reconnected to network?
- Poll `public/stop_departures`
  - What data needs to be included, starting with less obvious
    - `is_real_time`
    - `route_color`
    - Alert granularity?
  - What data needs to be join in for the display. My departure time endpoint should be
    - It seems like this has pretty much everything!
  denormalized but as minimal as possible otherwise.


Example call to nearby stops

```
curl -H "apiKey: $TRANSIT_KEY" https://external.transitapp.com/v3/public/nearby_stops -G -d "max_distance=1000&lat=40.68716&lon=-73.97465"| jq
```

Then taking an example stop at random from the output
```json
{
  "city_name": "",
  "distance": 354,
  "global_stop_id": "MTAS:18774",
  "location_type": 0,
  "parent_station": {
    "city_name": "",
    "global_stop_id": "MTAS:17558",
    "location_type": 1,
    "rt_stop_id": "235",
    "station_code": "",
    "station_name": "Atlantic Av-Barclays Ctr"
  },
  "parent_station_global_stop_id": "MTAS:17558",
  "route_type": 1,
  "rt_stop_id": "D24S",
  "stop_code": "",
  "stop_lat": 40.68446057580319,
  "stop_lon": -73.9768857083957,
  "stop_name": "Atlantic Av-Barclays Ctr",
  "wheelchair_boarding": 1
}
```

Then get departures with
```
curl -H "apiKey: $TRANSIT_KEY" https://external.transitapp.com/v3/public/stop_departures -G -d "global_stop_id=MTAS:18774"| jq > notes/static/stop-departures-example.json
```


Another option is `/v3/public/nearby_routes` which does both of these in one call.

```
curl -H "apiKey: $TRANSIT_KEY" https://external.transitapp.com/v3/public/nearby_routes -G -d "max_distance=900&lat=29.72134736791465&lon=-95.38383198936232" | jq > notes/static/nearby-routes-example.json
```

## StopDeparturesResponse Data Structures

### Main Response Structure
- **`StopDeparturesResponse`** - Top-level response containing an array of route departures

### Route-related Structs
- **`Route`** - Represents a transit route with all its properties (color, name, type, network info, itineraries)
- **`DisplayShortName`** - Visual identity for route display (boxed_text, elements, route_name_redundancy)
- **`Fare`** - Fare information for a route (fare_media_type, price_min, price_max)
- **`Price`** - Price details (currency_code, symbol, text, value)
- **`Vehicle`** - Vehicle information (image, name, name_inflection)
- **`ServiceAlert`** - Service alerts affecting routes (effect, severity, description, etc.)

### Itinerary-related Structs
- **`Itinerary`** - Group of trips with same stop pattern and direction (headsign, direction_id, schedule_items, branch_code)
- **`ScheduleItem`** - Individual departure time with real-time info (departure_time, is_real_time, is_cancelled, rt_trip_id, wheelchair_accessible, trip_search_key)

### Supporting Structs (some already exist)
- **`Stop`** - Already defined, represents a transit stop
- **`ParentStation`** - Already defined, station containing a stop

### ID/Reference Types
- **`GlobalRouteId`** - String type for global route identifiers
- **`TripSearchKey`** - String type for trip search keys
- **`DirectionId`** - Integer representing direction (0 or 1)



Actually scratch a lot of the above; there is a `/v3/public/nearby_routes` endpoint that
can cut out the need to query nearby_stops and stop_departures.

I'm pretty sure this is new...

I'm going to need to rewrite the client unless there's some reason not to use it. Looks
pretty good.
