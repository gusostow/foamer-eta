use anyhow::Result;
use messages::Client;

#[tokio::main]
async fn main() -> Result<()> {
    let client = Client::from_env().await?;
    for message in client.list_all().await? {
        println!("{message:?}");
    }
    Ok(())
}
