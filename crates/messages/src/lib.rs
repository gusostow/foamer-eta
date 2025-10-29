use anyhow::{Context, Result};
use aws_sdk_dynamodb::types::AttributeValue;
use aws_sdk_dynamodb::Client as DynamoClient;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
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

/// Client for managing messages in DynamoDB
#[derive(Clone)]
pub struct Client {
    table: String,
    dynamo: DynamoClient,
}

impl Client {
    /// Create a new messages client
    pub fn new(table: String, dynamo: DynamoClient) -> Self {
        Self { table, dynamo }
    }

    /// Create client from environment variables
    pub async fn from_env() -> Result<Self> {
        let table = std::env::var("FOAMER_MESSAGES_TABLE")
            .context("FOAMER_MESSAGES_TABLE environment variable not set")?;

        let config = aws_config::load_from_env().await;
        let dynamo = DynamoClient::new(&config);

        Ok(Self::new(table, dynamo))
    }

    /// Store or update a message
    pub async fn put(&self, content: String) -> Result<Message> {
        let message = Message::new(content);

        self.dynamo
            .put_item()
            .table_name(&self.table)
            .item("id", AttributeValue::S(message.id.clone()))
            .item("content", AttributeValue::S(message.content.clone()))
            .item(
                "created_at",
                AttributeValue::S(message.created_at.to_rfc3339()),
            )
            .send()
            .await
            .context("Failed to put message")?;

        Ok(message)
    }

    /// Retrieve a message by ID
    pub async fn get(&self, id: &str) -> Result<Message> {
        let result = self
            .dynamo
            .get_item()
            .table_name(&self.table)
            .key("id", AttributeValue::S(id.to_string()))
            .send()
            .await
            .context("Failed to get message")?;

        let item = result
            .item
            .ok_or_else(|| anyhow::anyhow!("Message not found: {}", id))?;

        self.parse_message(item)
    }

    /// List all messages
    pub async fn list_all(&self) -> Result<Vec<Message>> {
        let result = self
            .dynamo
            .scan()
            .table_name(&self.table)
            .send()
            .await
            .context("Failed to list messages")?;

        let mut messages: Vec<Message> = result
            .items
            .unwrap_or_default()
            .into_iter()
            .filter_map(|item| self.parse_message(item).ok())
            .collect();

        // Sort by creation time (newest first)
        messages.sort_by(|a, b| b.created_at.cmp(&a.created_at));

        Ok(messages)
    }

    /// Delete a message by ID
    pub async fn delete(&self, id: &str) -> Result<bool> {
        let result = self
            .dynamo
            .delete_item()
            .table_name(&self.table)
            .key("id", AttributeValue::S(id.to_string()))
            .return_values(aws_sdk_dynamodb::types::ReturnValue::AllOld)
            .send()
            .await
            .context("Failed to delete message")?;

        Ok(result.attributes.is_some())
    }

    /// Parse DynamoDB item into Message
    fn parse_message(
        &self,
        item: std::collections::HashMap<String, AttributeValue>,
    ) -> Result<Message> {
        let id = item
            .get("id")
            .and_then(|v| v.as_s().ok())
            .ok_or_else(|| anyhow::anyhow!("Missing or invalid id"))?
            .clone();

        let content = item
            .get("content")
            .and_then(|v| v.as_s().ok())
            .ok_or_else(|| anyhow::anyhow!("Missing or invalid content"))?
            .clone();

        let created_at = item
            .get("created_at")
            .and_then(|v| v.as_s().ok())
            .ok_or_else(|| anyhow::anyhow!("Missing or invalid created_at"))?;

        let created_at = DateTime::parse_from_rfc3339(created_at)
            .context("Failed to parse created_at")?
            .with_timezone(&Utc);

        Ok(Message {
            id,
            content,
            created_at,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn test_put_and_get() {
        let client = Client::from_env().await.unwrap();
        let message = client.put("Hello, world!".to_string()).await.unwrap();

        let retrieved = client.get(&message.id).await.unwrap();

        assert_eq!(retrieved.id, message.id);
        assert_eq!(retrieved.content, "Hello, world!");
        assert_eq!(retrieved.created_at, message.created_at);
    }

    #[tokio::test]
    async fn test_get_nonexistent() {
        let client = Client::from_env().await.unwrap();
        let result = client.get("nonexistent").await;
        assert!(result.is_err());
    }

    #[tokio::test]
    async fn test_list_all() {
        let client = Client::from_env().await.unwrap();

        let msg1 = client.put("First".to_string()).await.unwrap();
        // Sleep briefly to ensure different timestamps
        tokio::time::sleep(tokio::time::Duration::from_millis(10)).await;
        let msg2 = client.put("Second".to_string()).await.unwrap();

        let all = client.list_all().await.unwrap();
        assert!(all.len() >= 2);

        // Find our messages in the list
        let found_msg1 = all.iter().find(|m| m.id == msg1.id).unwrap();
        let found_msg2 = all.iter().find(|m| m.id == msg2.id).unwrap();

        assert_eq!(found_msg1.content, "First");
        assert_eq!(found_msg2.content, "Second");
    }

    #[tokio::test]
    async fn test_delete() {
        let client = Client::from_env().await.unwrap();
        let message = client.put("To be deleted".to_string()).await.unwrap();

        assert!(client.get(&message.id).await.is_ok());

        let deleted = client.delete(&message.id).await.unwrap();
        assert!(deleted);
        assert!(client.get(&message.id).await.is_err());

        // Deleting again should return false
        let deleted_again = client.delete(&message.id).await.unwrap();
        assert!(!deleted_again);
    }
}
