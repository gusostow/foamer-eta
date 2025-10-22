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


_2025-10-13_

I've had my suspicious that the ESP32-C3 won't cut it for the LED matrix displays.

I posted on reddit and that's somewhat confirmed https://www.reddit.com/r/esp32/comments/1o47emo/guidance_for_hub75_led_matrix_panel/.

An easier option is a combined board that's designed to plug directly into displays with HUB75.

I bought this from MicroCenter https://learn.adafruit.com/adafruit-matrixportal-s3.

Note that it isn't natively Rust compatible. I think doing this in Arduino or
CircuitPython/micropython.
