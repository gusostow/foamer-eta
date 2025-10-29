use anyhow::Result;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::sync::{Arc, RwLock};
use uuid::Uuid;

/// A message to be displayed on the LED sign
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message {
    /// Unique identifier for the message
    pub id: String,
    /// The message content to display
    pub content: String,
    /// Timestamp when the message was created
    pub created_at: DateTime<Utc>,
}

impl Message {
    /// Create a new message with auto-generated ID and current timestamp
    pub fn new(content: String) -> Self {
        Self {
            id: Uuid::new_v4().to_string(),
            content,
            created_at: Utc::now(),
        }
    }
}

/// Client for managing messages in memory
#[derive(Clone)]
pub struct Client {
    messages: Arc<RwLock<HashMap<String, Message>>>,
}

impl Client {
    /// Create a new messages client with empty storage
    pub fn new() -> Self {
        Self {
            messages: Arc::new(RwLock::new(HashMap::new())),
        }
    }

    /// Store or update a message
    pub fn put(&self, content: String) -> Result<Message> {
        let mut messages = self.messages.write().unwrap();
        let message = Message::new(content);
        messages.insert(message.id.clone(), message.clone());
        Ok(message)
    }

    /// Retrieve a message by ID
    pub fn get(&self, id: &str) -> Result<Message> {
        let messages = self.messages.read().unwrap();
        messages
            .get(id)
            .cloned()
            .ok_or_else(|| anyhow::anyhow!("Message not found: {}", id))
    }

    /// List all messages
    pub fn list_all(&self) -> Result<Vec<Message>> {
        let messages = self.messages.read().unwrap();
        let mut all_messages: Vec<Message> = messages.values().cloned().collect();

        // Sort by creation time (newest first)
        all_messages.sort_by(|a, b| b.created_at.cmp(&a.created_at));

        Ok(all_messages)
    }

    /// Delete a message by ID
    pub fn delete(&self, id: &str) -> Result<bool> {
        let mut messages = self.messages.write().unwrap();
        Ok(messages.remove(id).is_some())
    }
}

impl Default for Client {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_put_and_get() {
        let client = Client::new();
        let message = client.put("Hello, world!".to_string()).unwrap();

        let retrieved = client.get(&message.id).unwrap();

        assert_eq!(retrieved.id, message.id);
        assert_eq!(retrieved.content, "Hello, world!");
        assert_eq!(retrieved.created_at, message.created_at);
    }

    #[test]
    fn test_get_nonexistent() {
        let client = Client::new();
        let result = client.get("nonexistent");
        assert!(result.is_err());
    }

    #[test]
    fn test_list_all() {
        let client = Client::new();

        let msg1 = client.put("First".to_string()).unwrap();
        // Sleep briefly to ensure different timestamps
        std::thread::sleep(std::time::Duration::from_millis(10));
        let msg2 = client.put("Second".to_string()).unwrap();

        let all = client.list_all().unwrap();
        assert_eq!(all.len(), 2);

        // Should be sorted newest first
        assert_eq!(all[0].id, msg2.id);
        assert_eq!(all[1].id, msg1.id);
    }

    #[test]
    fn test_delete() {
        let client = Client::new();
        let message = client.put("To be deleted".to_string()).unwrap();

        assert!(client.get(&message.id).is_ok());

        let deleted = client.delete(&message.id).unwrap();
        assert!(deleted);
        assert!(client.get(&message.id).is_err());

        // Deleting again should return false
        let deleted_again = client.delete(&message.id).unwrap();
        assert!(!deleted_again);
    }
}
