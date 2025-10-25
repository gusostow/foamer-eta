use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use transit::TransitClient;

pub mod svc;

// in meters
const DEFAULT_DISTANCE: u32 = 500;

type LatLon = (f32, f32);

#[derive(Debug, Serialize, Deserialize)]
pub struct Departures {
    pub routes: Vec<Route>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Route {
    pub name: String,
    pub mode: String,
    pub color: String,
    pub directions: Vec<Direction>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Direction {
    pub headsign: String,
    pub departures: Vec<Departure>,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type", content = "minutes")]
pub enum Departure {
    Scheduled(u16),
    RealTime(u16),
}

pub struct Client {
    transit_client: TransitClient,
}

impl Client {
    pub fn new() -> Result<Self> {
        Ok(Self {
            transit_client: TransitClient::from_env()?,
        })
    }

    pub async fn departures(
        &self,
        coords: &LatLon,
        max_distance: Option<u32>,
    ) -> Result<Departures> {
        let (lat, lon) = coords;

        let response = self
            .transit_client
            .nearby_routes(
                *lat as f64,
                *lon as f64,
                max_distance.or(Some(DEFAULT_DISTANCE)),
                Some(true),
                None,
                None,
            )
            .await?;

        let routes: Result<Vec<_>> = response
            .routes
            .into_iter()
            .map(|route| {
                let directions: Result<Vec<_>> = route
                    .itineraries
                    .into_iter()
                    .map(|itinerary| {
                        let departures: Result<Vec<_>> = itinerary
                            .schedule_items
                            .into_iter()
                            .filter_map(|item| {
                                let now = std::time::SystemTime::now()
                                    .duration_since(std::time::UNIX_EPOCH)
                                    .context("Failed to get current time")
                                    .ok()?
                                    .as_secs();

                                // Calculate minutes, handling negative values (past departures)
                                let minutes = if item.departure_time > now {
                                    ((item.departure_time - now) / 60).min(u16::MAX as u64) as u16
                                } else {
                                    0
                                };

                                // Filter out departures > 60 minutes
                                if minutes > 99 {
                                    return None;
                                }

                                Some(Ok(if item.is_real_time {
                                    Departure::RealTime(minutes)
                                } else {
                                    Departure::Scheduled(minutes)
                                }))
                            })
                            .collect();

                        Ok(Direction {
                            headsign: itinerary.headsign,
                            departures: departures?,
                        })
                    })
                    .collect();

                Ok(Route {
                    name: if !route.route_short_name.is_empty() {
                        route.route_short_name
                    } else {
                        route.route_long_name
                    },
                    mode: route
                        .mode_name
                        .unwrap_or_else(|| format!("type_{}", route.route_type)),
                    color: route.route_color,
                    directions: directions?,
                })
            })
            .collect();

        let routes = routes?;

        Ok(Departures { routes })
    }
}
