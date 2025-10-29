# Message subsystem

The goal is to store and periodically serve short custom messages to the LED display.

Messages will be sourced by a simple web frontend and posted to a backend endpoint, which will be
hosted by the same Lambda function that runs the departure times API.

## Storage

For simplicity messages will be stored using DynamoDB.

## Code organization

New crate `messages` with client that can put a new message, get a message based on some key,
and query all messages.

`svc` crate will use `messages` crate to create a new endpoint for posting/putting new messages. The
endpoint does not need to be able to query messages.

## Security

To prevent abuse, it's important to limit who can add new messages. Let's use a simple shared
secret.

The secret will need to gate access at two points.
- Initial access to the frontend.
- Making API calls.
