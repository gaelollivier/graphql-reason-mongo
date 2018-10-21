open Graphql_lwt;
open ReMongo;

module User = {
  type t = {
    id: string,
    email: string,
    firstname: string,
    lastname: string,
  };
};

module CreateUserInput = {
  type t = {
    email: string,
    firstname: string,
    lastname: string,
  };
};

module CreateUserPayload = {
  type t = {user: Bson.t};
};

/* GraphQL Schema types */
let objectId =
  Schema.scalar(
    ~doc="MongoDB object ID", "ObjectId", ~coerce=(id: ObjectId.t) =>
    `String(ObjectId.toString(id))
  );

let user =
  Schema.(
    obj("User", ~fields=_ =>
      User.[
        field(
          "id", ~args=Arg.[], ~typ=non_null(objectId), ~resolve=((), doc) =>
          Bson.getElement("_id", doc) |> Bson.getObjectId
        ),
        field(
          "email", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), doc) =>
          Bson.getElement("email", doc) |> Bson.getString
        ),
        field(
          "firstname",
          ~args=Arg.[],
          ~typ=non_null(string),
          ~resolve=((), doc) =>
          Bson.getElement("firstname", doc) |> Bson.getString
        ),
        field(
          "lastname", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), doc) =>
          Bson.getElement("lastname", doc) |> Bson.getString
        ),
      ]
    )
  );

let createUserInput =
  Schema.Arg.(
    obj(
      "CreateUserInput",
      ~fields=[
        arg("email", ~typ=non_null(string)),
        arg("firstname", ~typ=non_null(string)),
        arg("lastname", ~typ=non_null(string)),
      ],
      ~coerce=(email, firstname, lastname) =>
      CreateUserInput.{email, firstname, lastname}
    )
  );

let createUserPayload =
  Schema.(
    obj("CreateUserPayload", ~fields=_ =>
      CreateUserPayload.[
        field(
          "user", ~args=Arg.[], ~typ=non_null(user), ~resolve=((), payload) =>
          payload.user
        ),
      ]
    )
  );

let connectAndStart = () => {
  /* connect to localhost Mongo db with default port */
  let%lwt usersCollection = Mongo_lwt.createLocalDefault("db", "users");

  let schema =
    Schema.(
      schema(
        ~mutation_name="Mutation",
        ~mutations=[
          io_field(
            "createUser",
            ~args=Arg.[arg(~typ=non_null(createUserInput), "input")],
            ~typ=non_null(createUserPayload),
            /* users resolver */
            ~resolve=((), (), input) => {
              let {email, firstname, lastname}: CreateUserInput.t = input;
              /* create & insert mongo doc */
              let newUserDoc =
                Bson.fromElements([
                  ("_id", Bson.createObjectId(ObjectId.generate())),
                  ("email", Bson.createString(email)),
                  ("firstname", Bson.createString(firstname)),
                  ("lastname", Bson.createString(lastname)),
                ]);
              let%lwt () = Mongo_lwt.insert(usersCollection, [newUserDoc]);
              Lwt.return_ok(CreateUserPayload.{user: newUserDoc});
            },
          ),
        ],
        ~query_name="Query",
        [
          /* Query fields */
          io_field(
            "users",
            ~args=Arg.[],
            ~typ=non_null(list(non_null(user))),
            /* users resolver */
            ~resolve=((), ()) => {
              let%lwt res = Mongo_lwt.find(usersCollection);
              let docs = res |> MongoReply.getDocumentList;

              Lwt.return_ok(docs);
            },
          ),
        ],
      )
    );

  Server.start(~ctx=req => (), schema);
};

let start = () => Lwt_main.run(connectAndStart());