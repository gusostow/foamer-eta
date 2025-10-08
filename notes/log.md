_2025-10-06_

I need a rust client for transit api. There's an OpenAPI spec so I could do codegen, but
I probably shouldn't bring on more complexity than I need to.

I'll try writing it by hand with LLM help.


_2025-10-08_


I got worried that Transit wouldn't have real-time departures near the location in
question.

But I checked there's some.

```
jq '[.routes[].itineraries[].schedule_items[]] | (map(select(.is_real_time == true)) | length) / length' notes/static/nearby-routes-example.json
0.2777777777777778
```

