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
            stop.distance <= 1000,
            "Stop distance {} exceeds max_distance 1000",
            stop.distance
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
