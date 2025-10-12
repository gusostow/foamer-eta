use anyhow::Result;

#[tokio::main]
async fn main() -> Result<()> {
    let app = api::svc::create_router()?;

    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await?;
    println!("Server listening on {}", listener.local_addr()?);

    axum::serve(listener, app).await?;

    Ok(())
}
