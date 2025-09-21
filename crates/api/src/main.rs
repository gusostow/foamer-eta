use api::gtfs::FeedMessage;
use anyhow::Result;
use prost::Message;
use std::env;

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<()> {
    println!("GTFS Realtime API ready!");

    // Get API key from environment
    let api_key = env::var("RIDE_METRO_KEY")
        .expect("RIDE_METRO_KEY environment variable must be set");

    // Make HTTP request to GTFS Realtime TripUpdates endpoint
    let client = reqwest::Client::new();
    let response = client
        .get("https://api.ridemetro.org/GtfsRealtime/TripUpdates")
        .header("Cache-Control", "no-cache")
        .header("Ocp-Apim-Subscription-Key", api_key)
        .send()
        .await?;

    println!("Response status: {}", response.status());

    // Get the protobuf bytes
    let bytes = response.bytes().await?;
    println!("Received {} bytes of protobuf data", bytes.len());

    // Parse the protobuf data into our FeedMessage
    match FeedMessage::decode(bytes) {
        Ok(feed_message) => {
            println!("Successfully parsed GTFS Realtime feed!");
            println!("Number of entities: {}", feed_message.entity.len());

            // Print first few trip updates for demonstration
            for (i, entity) in feed_message.entity.iter().take(1).enumerate() {
                if let Some(trip_update) = &entity.trip_update {
                    println!("\n=== Trip Update {} ===", i + 1);
                    println!("Entity ID: {}", entity.id);
                    println!("Trip Update: {:#?}", trip_update);
                }
            }
        }
        Err(e) => {
            eprintln!("Failed to parse protobuf data: {}", e);
        }
    }

    Ok(())
}
