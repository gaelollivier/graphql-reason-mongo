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
  type t = {user: User.t};
};

/* GraphQL Schema types */
let objectId =
  Schema.scalar(~doc="MongoDB object ID", "ObjectId", ~coerce=(id: string)
    /* format ObjectIDs as hexadecimal */
    => `String(Hex.show(Hex.of_string(id))));

let user =
  Schema.(
    obj("User", ~fields=_ =>
      User.[
        field("id", ~args=Arg.[], ~typ=non_null(objectId), ~resolve=((), u) =>
          u.id
        ),
        field("email", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), u) =>
          u.email
        ),
        field(
          "firstname", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), u) =>
          u.firstname
        ),
        field(
          "lastname", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), u) =>
          u.lastname
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
  let%lwt usersCollection = Mongo_lwt.create_local_default("db", "users");

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
                Bson.empty
                |> Bson.add_element("email", Bson.create_string(email))
                |> Bson.add_element(
                     "firstname",
                     Bson.create_string(firstname),
                   )
                |> Bson.add_element("lastname", Bson.create_string(lastname));
              let%lwt () = Mongo_lwt.insert(usersCollection, [newUserDoc]);
              /* TODO: retrieve inserted id */
              let newUser = User.{id: "", email, firstname, lastname};
              Lwt.return_ok(CreateUserPayload.{user: newUser});
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
              let docs =
                res
                |> MongoReply.get_document_list
                |> List.map(doc =>
                     User.{
                       id: Bson.get_element("_id", doc) |> Bson.get_objectId,
                       email:
                         Bson.get_element("email", doc) |> Bson.get_string,
                       firstname:
                         Bson.get_element("firstname", doc) |> Bson.get_string,
                       lastname:
                         Bson.get_element("lastname", doc) |> Bson.get_string,
                     }
                   );
              Lwt.return_ok(docs);
            },
          ),
        ],
      )
    );

  Server.start(~ctx=req => (), schema);
};

let start = () => Lwt_main.run(connectAndStart());