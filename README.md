# ReasonML GraphQL Server example

## Build

```
esy
esy build
```

## Run

```
esy x Server
```

Then open `http://localhost:8080/graphql` in a web browser, or query it with your preferred GraphQL client.

## Database

Example assumes a MongoDB instance running on localhost with default port (`27017`). It uses the collection `users` in database `db`.
