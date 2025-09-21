Need to select a [GTFS (general transit feed specification)](https://gtfs.org/getting-started/what-is-GTFS/)
service to supply data.

GTFS supports schedule data and realtime. Do these need to be integrated on the client side?

https://metro.resourcespace.com/pages/search.php?search=%21collection241

# Options

## 1. Transit app

https://transitapp.com/apis

This seems perfect and has a free tier if you stay under a certain number of API calls.

The free tier only allows 1500 API calls per month. 

3 transit stops x 1 poll per minute x 18h / day = 97,200 calls / mo. Plus a handful of
low frequency calls to refresh local stops etc.

I emailed them begging for more access.

## 2. Houston Metro API

https://api-portal.ridemetro.org/

Free but Houston only. Not sure about it's accuracy compared to Transit App. I subscribed.
API key in uncomitted `.envrc.local`.

Static GTFS data https://metro.resourcespace.com/pages/search.php?search=%21collection241.

APIs

- GTFS realtime (protobuf)
- Transit data: routes, stops, schedules etc
- Service alerts (JSON or protobuf)

This should be base case - we only _need_ to support Houston. If I end up wanting to
produce more of these, I can standardize the output and connect to other providers.


Call protobuf endpoint with (binary output obviously):
```
$ curl -X GET "https://api.ridemetro.org/GtfsRealtime/TripUpdates" -H "Cache-Control: no-cache" -H "Ocp-Apim-Subscription-Key: $RIDE_METRO_KEY"
```
