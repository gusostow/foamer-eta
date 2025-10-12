use anyhow::Result;
use api::Client;

#[tokio::test]
async fn test_departures() -> Result<()> {
    let coords = (29.72134736791465, -95.38383198936232);

    let client = Client::new()?;
    let departures = client.departures(&coords, None).await?;

    // Verify we got some routes
    assert!(
        !departures.routes.is_empty(),
        "Should have at least one route"
    );

    // Check that each route has expected fields
    for route in &departures.routes {
        assert!(!route.name.is_empty(), "Route name should not be empty");
        assert!(!route.mode.is_empty(), "Route mode should not be empty");
        assert!(!route.color.is_empty(), "Route color should not be empty");
        assert!(
            !route.directions.is_empty(),
            "Route should have at least one direction"
        );

        // Check directions
        for direction in &route.directions {
            assert!(
                !direction.headsign.is_empty(),
                "Direction headsign should not be empty"
            );
            // Departures might be empty if no upcoming service
        }
    }

    println!("Found {} routes", departures.routes.len());
    for route in &departures.routes {
        println!("Route: {} ({})", route.name, route.mode);
        for direction in &route.directions {
            println!(
                "  → {}: {} departures",
                direction.headsign,
                direction.departures.len()
            );
        }
    }

    Ok(())
}

#[tokio::test]
async fn test_departures_zero_distance() -> Result<()> {
    let coords = (29.72134736791465, -95.38383198936232);

    let client = Client::new()?;
    let departures = client.departures(&coords, Some(0)).await?;

    // With max_distance of 0, no routes should be returned
    assert!(
        departures.routes.is_empty(),
        "Should have no routes with max_distance of 0"
    );

    Ok(())
}
