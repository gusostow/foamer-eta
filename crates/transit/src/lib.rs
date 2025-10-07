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
        let url = format!("{}/public/stop_departures", BASE_URL);

        let response = self
            .client
            .get(&url)
            .header("apiKey", &self.api_key)
            .query(&[("global_stop_id", global_stop_id)])
            .send()
            .await?
            .error_for_status()?
            .json()
            .await?;

        Ok(response)
    }
}

// Response types based on notes/transit-api.md

#[derive(Debug, Serialize, Deserialize)]
pub struct NearbyStopsResponse {
    pub stops: Vec<Stop>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct StopDeparturesResponse {
    pub route_departures: Vec<Route>,
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

// Route-related types

#[derive(Debug, Serialize, Deserialize)]
pub struct Route {
    pub global_route_id: GlobalRouteId,
    pub itineraries: Vec<Itinerary>,
    pub route_long_name: String,
    pub route_short_name: String,
    pub route_type: u8,
    pub route_color: String,
    pub route_text_color: String,
    // Optional fields
    pub alerts: Option<Vec<ServiceAlert>>,
    pub route_timezone: Option<String>,
    pub route_display_short_name: Option<DisplayShortName>,
    pub compact_display_short_name: Option<DisplayShortName>,
    pub fares: Option<Vec<Fare>>,
    pub route_network_name: Option<String>,
    pub route_network_id: Option<String>,
    pub tts_long_name: Option<String>,
    pub tts_short_name: Option<String>,
    pub sorting_key: Option<String>,
    pub mode_name: Option<String>,
    pub real_time_route_id: Option<String>,
    pub vehicle: Option<Vehicle>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct DisplayShortName {
    pub elements: Vec<Option<String>>,
    pub route_name_redundancy: bool,
    pub boxed_text: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Fare {
    pub fare_media_type: u8,
    pub price_min: Price,
    pub price_max: Price,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Price {
    pub currency_code: String,
    pub symbol: String,
    pub text: String,
    pub value: f64,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Vehicle {
    pub name: Option<String>,
    pub name_inflection: Option<String>,
    pub image: Option<String>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ServiceAlert {
    pub effect: String,
    pub severity: String,
    pub description: String,
    pub created_at: u64,
    pub informed_entities: Vec<InformedEntity>,
    pub title: Option<String>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct InformedEntity {
    pub global_route_id: Option<GlobalRouteId>,
    pub global_stop_id: Option<String>,
    pub rt_trip_id: Option<String>,
}

// Itinerary-related types

#[derive(Debug, Serialize, Deserialize)]
pub struct Itinerary {
    pub direction_id: DirectionId,
    pub headsign: String,
    pub schedule_items: Vec<ScheduleItem>,
    // Optional fields
    pub direction_headsign: Option<String>,
    pub merged_headsign: Option<String>,
    pub branch_code: Option<String>,
    pub closest_stop: Option<Stop>,
    pub stops: Option<Vec<Stop>>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ScheduleItem {
    pub departure_time: u64,
    pub is_cancelled: bool,
    pub is_real_time: bool,
    pub scheduled_departure_time: u64,
    // Optional fields
    pub rt_trip_id: Option<String>,
    pub wheelchair_accessible: Option<u8>,
    pub trip_search_key: Option<TripSearchKey>,
}

// ID/Reference types

pub type GlobalRouteId = String;
pub type TripSearchKey = String;
pub type DirectionId = u8;
