use anyhow::Result;

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<()> {
    let client = transit::TransitClient::from_env()?;
    let response = client.stop_departures("MTAS:18774").await?;
    println!("{:#?}", response);
    Ok(())
}
