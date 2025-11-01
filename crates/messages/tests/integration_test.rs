use messages::Client;

#[tokio::test]
async fn test_put_and_get() {
    let client = Client::from_env().await.unwrap();
    let message = client.put("hello, world!".to_string()).await.unwrap();

    let retrieved = client.get(&message.id).await.unwrap();

    assert_eq!(retrieved.id, message.id);
    assert_eq!(retrieved.content, "hello, world!");
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

    let msg1 = client.put("first".to_string()).await.unwrap();
    // sleep briefly to ensure different timestamps
    tokio::time::sleep(tokio::time::Duration::from_millis(10)).await;
    let msg2 = client.put("second".to_string()).await.unwrap();

    let all = client.list_all().await.unwrap();
    assert!(all.len() >= 2);

    // find our messages in the list
    let found_msg1 = all.iter().find(|m| m.id == msg1.id).unwrap();
    let found_msg2 = all.iter().find(|m| m.id == msg2.id).unwrap();

    assert_eq!(found_msg1.content, "first");
    assert_eq!(found_msg2.content, "second");
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
