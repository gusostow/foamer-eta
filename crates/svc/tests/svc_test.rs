use anyhow::Result;
use api::Departures;
use axum::body::Body;
use axum::http::{Request, StatusCode};
use tower::ServiceExt;

#[tokio::test]
async fn test_departures_endpoint() -> Result<()> {
    let app = svc::create_router()?;

    let response = app
        .oneshot(
            Request::builder()
                .uri("/departures?lat=29.72134736791465&lon=-95.38383198936232&max_distance=500")
                .body(Body::empty())?,
        )
        .await?;

    assert_eq!(response.status(), StatusCode::OK);

    let body = axum::body::to_bytes(response.into_body(), usize::MAX).await?;
    let departures: Departures = serde_json::from_slice(&body)?;

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
        }
    }

    Ok(())
}

#[tokio::test]
async fn test_departures_endpoint_default_distance() -> Result<()> {
    let app = svc::create_router()?;

    let response = app
        .oneshot(
            Request::builder()
                .uri("/departures?lat=29.72134736791465&lon=-95.38383198936232")
                .body(Body::empty())?,
        )
        .await?;

    assert_eq!(response.status(), StatusCode::OK);

    let body = axum::body::to_bytes(response.into_body(), usize::MAX).await?;
    let departures: Departures = serde_json::from_slice(&body)?;

    // Should use default distance and return routes
    assert!(
        !departures.routes.is_empty(),
        "Should have routes with default distance"
    );

    Ok(())
}

#[tokio::test]
async fn test_departures_endpoint_missing_params() -> Result<()> {
    let app = svc::create_router()?;

    let response = app
        .oneshot(
            Request::builder()
                .uri("/departures?lat=29.72134736791465")
                .body(Body::empty())?,
        )
        .await?;

    // Should return 400 Bad Request when required params are missing
    assert_eq!(response.status(), StatusCode::BAD_REQUEST);

    Ok(())
}
