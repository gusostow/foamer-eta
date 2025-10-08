use transit::TransitClient;

#[tokio::test]
async fn test_nearby_stops() {
    let client = TransitClient::from_env().expect("Failed to create client from environment");

    // Using coordinates from notes/transit-api.md example
    let result = client.nearby_stops(40.68716, -73.97465, 1000).await;

    assert!(result.is_ok(), "Request should succeed: {:?}", result.err());

    let response = result.unwrap();

    // Should return at least one stop within 1000m of this location
    assert!(
        !response.stops.is_empty(),
        "Should have at least one nearby stop"
    );

    // All stops should be within the max_distance
    for stop in &response.stops {
        assert!(
            stop.distance.is_some(),
            "Stop should have a distance in nearby_stops response"
        );
        let distance = stop.distance.unwrap();
        assert!(
            distance <= 1000,
            "Stop distance {} exceeds max_distance 1000",
            distance
        );
    }

    // All stops should have valid global_stop_ids
    for stop in &response.stops {
        assert!(
            !stop.global_stop_id.is_empty(),
            "Stop should have a global_stop_id"
        );
    }
}

#[tokio::test]
async fn test_stop_departures() {
    let client = TransitClient::from_env().expect("Failed to create client from environment");

    // Using global_stop_id from notes/transit-api.md example
    let result = client.stop_departures("MTAS:18774").await;

    assert!(result.is_ok(), "Request should succeed: {:?}", result.err());

    let response = result.unwrap();

    // Should return at least one route departure
    assert!(
        !response.route_departures.is_empty(),
        "Should have at least one route departure"
    );

    // Check first route has required fields
    let first_route = &response.route_departures[0];
    assert!(
        !first_route.global_route_id.is_empty(),
        "Route should have a global_route_id"
    );
    assert!(
        !first_route.route_long_name.is_empty(),
        "Route should have a long name"
    );
    assert!(
        !first_route.route_short_name.is_empty(),
        "Route should have a short name"
    );
    assert!(
        !first_route.route_color.is_empty(),
        "Route should have a color"
    );
    assert!(
        !first_route.route_text_color.is_empty(),
        "Route should have a text color"
    );

    // Check route has itineraries
    assert!(
        !first_route.itineraries.is_empty(),
        "Route should have at least one itinerary"
    );

    // Check first itinerary has required fields
    let first_itinerary = &first_route.itineraries[0];
    assert!(
        !first_itinerary.headsign.is_empty(),
        "Itinerary should have a headsign"
    );

    // Check itinerary has schedule items
    assert!(
        !first_itinerary.schedule_items.is_empty(),
        "Itinerary should have at least one schedule item"
    );

    // Check first schedule item has valid departure times
    let first_schedule_item = &first_itinerary.schedule_items[0];
    assert!(
        first_schedule_item.departure_time > 0,
        "Schedule item should have a valid departure time"
    );
    assert!(
        first_schedule_item.scheduled_departure_time > 0,
        "Schedule item should have a valid scheduled departure time"
    );
}

#[tokio::test]
async fn test_nearby_routes() {
    let client = TransitClient::from_env().expect("Failed to create client from environment");

    // Using coordinates from notes/transit-api.md example
    let result = client
        .nearby_routes(
            29.72134736791465,
            -95.38383198936232,
            Some(900),
            None,
            None,
            None,
        )
        .await;

    assert!(result.is_ok(), "Request should succeed: {:?}", result.err());

    let response = result.unwrap();

    // Should return at least one route within 900m of this location
    assert!(
        !response.routes.is_empty(),
        "Should have at least one nearby route"
    );

    // Check first route has required fields
    let first_route = &response.routes[0];
    assert!(
        !first_route.global_route_id.is_empty(),
        "Route should have a global_route_id"
    );
    assert!(
        !first_route.route_long_name.is_empty(),
        "Route should have a long name"
    );
    assert!(
        !first_route.route_short_name.is_empty(),
        "Route should have a short name"
    );
    assert!(
        !first_route.route_color.is_empty(),
        "Route should have a color"
    );
    assert!(
        !first_route.route_text_color.is_empty(),
        "Route should have a text color"
    );

    // Check route has itineraries
    assert!(
        !first_route.itineraries.is_empty(),
        "Route should have at least one itinerary"
    );

    // Check first itinerary has required fields and closest_stop
    let first_itinerary = &first_route.itineraries[0];
    assert!(
        !first_itinerary.headsign.is_empty(),
        "Itinerary should have a headsign"
    );
    assert!(
        first_itinerary.closest_stop.is_some(),
        "Itinerary should have a closest_stop"
    );

    // Check itinerary has schedule items
    assert!(
        !first_itinerary.schedule_items.is_empty(),
        "Itinerary should have at least one schedule item"
    );

    // Check first schedule item has valid times
    let first_schedule_item = &first_itinerary.schedule_items[0];
    assert!(
        first_schedule_item.departure_time > 0,
        "Schedule item should have a valid departure time"
    );
    assert!(
        first_schedule_item.scheduled_departure_time > 0,
        "Schedule item should have a valid scheduled departure time"
    );
}
