use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};

const BASE_URL: &str = "https://external.transitapp.com/v3";

pub struct TransitClient {
    client: reqwest::Client,
    api_key: String,
}

impl TransitClient {
    pub fn new(api_key: String) -> Self {
        Self {
            client: reqwest::Client::new(),
            api_key,
        }
    }

    pub fn from_env() -> Result<Self> {
        let api_key =
            std::env::var("TRANSIT_KEY").context("TRANSIT_KEY environment variable not set")?;
        Ok(Self::new(api_key))
    }

    pub async fn nearby_stops(
        &self,
        lat: f64,
        lon: f64,
        max_distance: u32,
    ) -> Result<NearbyStopsResponse> {
        let url = format!("{}/public/nearby_stops", BASE_URL);

        let response = self
            .client
            .get(&url)
            .header("apiKey", &self.api_key)
            .query(&[
                ("lat", lat.to_string()),
                ("lon", lon.to_string()),
                ("max_distance", max_distance.to_string()),
            ])
            .send()
            .await?
            .error_for_status()?
            .json()
            .await?;

        Ok(response)
    }

    pub async fn stop_departures(&self, global_stop_id: &str) -> Result<StopDeparturesResponse> {
        todo!()
    }
}

// Response types based on notes/transit-api.md

#[derive(Debug, Serialize, Deserialize)]
pub struct NearbyStopsResponse {
    pub stops: Vec<Stop>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct StopDeparturesResponse {
    // TODO: Add fields
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Stop {
    pub city_name: String,
    pub distance: u32,
    pub global_stop_id: String,
    pub location_type: u8,
    pub parent_station: Option<ParentStation>,
    pub parent_station_global_stop_id: Option<String>,
    pub route_type: u8,
    pub rt_stop_id: String,
    pub stop_code: String,
    pub stop_lat: f64,
    pub stop_lon: f64,
    pub stop_name: String,
    pub wheelchair_boarding: u8,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ParentStation {
    pub city_name: String,
    pub global_stop_id: String,
    pub location_type: u8,
    pub rt_stop_id: String,
    pub station_code: String,
    pub station_name: String,
}
