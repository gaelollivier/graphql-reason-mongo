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

Then connect using your prefered GraphQL client on: `http://localhost:8080/graphql`

## Database

Example assumes a MongoDB instance running on localhost with default port (`27017`). It uses the collection `users` in database `db`.
