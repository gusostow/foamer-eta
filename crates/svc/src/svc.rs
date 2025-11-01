use api::{Client, Departures};
use anyhow::Result;
use axum::{
    extract::{Query, State},
    http::StatusCode,
    response::{IntoResponse, Response},
    routing::{get, post},
    Json, Router,
};
use messages::{Client as MessagesClient, Message};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use tower_http::trace::TraceLayer;

#[derive(Deserialize)]
pub struct DeparturesQuery {
    lat: f32,
    lon: f32,
    max_distance: Option<u32>,
}

pub struct AppState {
    client: Client,
    messages_client: MessagesClient,
}

pub async fn create_router() -> Result<Router> {
    let client = Client::new()?;
    let messages_client = MessagesClient::from_env().await?;
    let state = Arc::new(AppState {
        client,
        messages_client,
    });

    let app = Router::new()
        .route("/departures", get(get_departures))
        .route("/messages", post(post_message))
        .with_state(state)
        .layer(TraceLayer::new_for_http());

    Ok(app)
}

async fn get_departures(
    State(state): State<Arc<AppState>>,
    Query(params): Query<DeparturesQuery>,
) -> Result<Json<Departures>, AppError> {
    let coords = (params.lat, params.lon);
    let departures = state
        .client
        .departures(&coords, params.max_distance)
        .await?;

    Ok(Json(departures))
}

#[derive(Deserialize, Serialize)]
pub struct PostMessageRequest {
    pub content: String,
}

async fn post_message(
    State(state): State<Arc<AppState>>,
    Json(payload): Json<PostMessageRequest>,
) -> Result<Json<Message>, AppError> {
    // Create and store the message
    let message = state.messages_client.put(payload.content).await?;
    Ok(Json(message))
}

// Error handling
struct AppError(anyhow::Error);

impl IntoResponse for AppError {
    fn into_response(self) -> Response {
        tracing::error!("Request failed: {:?}", self.0);
        (
            StatusCode::INTERNAL_SERVER_ERROR,
            format!("Internal server error: {}", self.0),
        )
            .into_response()
    }
}

impl<E> From<E> for AppError
where
    E: Into<anyhow::Error>,
{
    fn from(err: E) -> Self {
        Self(err.into())
    }
}
