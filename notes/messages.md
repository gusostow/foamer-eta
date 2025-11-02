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


## Site

### Functionality

- Text box to submit text to /messages endpoint via POST request.
- Don't allow user to submit messages >96 chars.

### Stack

Simple static site in CSS, html, and potentically javascript.

### Code organization

Put files under `frontend/` at the root of the repo.

### Deployment

This will be deployed as a static site on Github Pages.

For local development I should just be able to open the HTML file in my web browser.

This will be deployed to two GH Pages sites for dev and prod. Each frontend should use the correct
Lambda API Gateway url from the CDK stack.

The `deploy-dev.yml` workflow will deploy to the dev Github pages site.
