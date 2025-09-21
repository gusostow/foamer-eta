use api::gtfs::FeedMessage;

fn main() {
    println!("GTFS Realtime API ready!");

    // Example: Create a new FeedMessage (this will fail at runtime without proper data)
    // but shows the generated types are available at compile time
    let _message = FeedMessage::default();
    println!("FeedMessage type is available for GTFS Realtime protobuf parsing");
}
