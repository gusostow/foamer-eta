use anyhow::Result;

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<()> {
    let client = transit::TransitClient::from_env()?;
    let response = client
        .nearby_routes(29.72134736791465, -95.38383198936232, Some(900), None, None, None)
        .await;
    println!("{:#?}", response);
    Ok(())
}
