open Graphql_lwt;

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

let connectAndStart = () => {
  /* connect to localhost Mongo db with default port */
  let%lwt usersCollection = Mongo_lwt.create_local_default("db", "users");

  let objectId =
    Schema.scalar(~doc="MongoDB object ID", "ObjectId", ~coerce=(id: string)
      /* format ObjectIDs as hexadecimal */
      => `String(Hex.show(Hex.of_string(id))));

  /* GraphQL User schema */
  let user =
    Schema.(
      obj("User", ~fields=user =>
        User.[
          field(
            "id", ~args=Arg.[], ~typ=non_null(objectId), ~resolve=((), p) =>
            p.id
          ),
          field(
            "email", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), p) =>
            p.email
          ),
          field(
            "firstname",
            ~args=Arg.[],
            ~typ=non_null(string),
            ~resolve=((), p) =>
            p.firstname
          ),
          field(
            "lastname", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), p) =>
            p.lastname
          ),
        ]
      )
    );

  let schema =
    Schema.(
      schema(
        ~mutation_name="Mutation",
        ~mutations=[
          io_field(
            "createUser",
            ~args=
              Arg.[
                arg(
                  ~typ=
                    non_null(
                      obj(
                        "CreateUserInput",
                        ~fields=[
                          arg("email", ~typ=non_null(string)),
                          arg("firstname", ~typ=non_null(string)),
                          arg("lastname", ~typ=non_null(string)),
                        ],
                        ~coerce=(email, firstname, lastname) =>
                        CreateUserInput.{email, firstname, lastname}
                      ),
                    ),
                  "input",
                ),
              ],
            ~typ=non_null(list(non_null(user))),
            /* users resolver */
            ~resolve=((), (), input) => {
              print_endline("HERE!");
              print_endline(input.email);
              print_endline(input.firstname);
              print_endline(input.lastname);
              /* TODO: create & insert doc :) */
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
                |> List.map(doc => {
                     let profile =
                       doc
                       |> Bson.get_element("profile")
                       |> Bson.get_doc_element;
                     User.{
                       id: Bson.get_element("_id", doc) |> Bson.get_objectId,
                       email:
                         Bson.get_element("email", doc) |> Bson.get_string,
                       firstname:
                         profile
                         |> Bson.get_element("firstname")
                         |> Bson.get_string,
                       lastname:
                         profile
                         |> Bson.get_element("lastname")
                         |> Bson.get_string,
                     };
                   });
              Lwt.return_ok(docs);
            },
          ),
        ],
      )
    );

  Server.start(~ctx=req => (), schema);
};

let start = () => Lwt_main.run(connectAndStart());