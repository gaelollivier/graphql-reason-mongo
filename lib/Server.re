open Graphql_lwt;

/* connect to localhost Mongo db with default port */
let usersCollection = Mongo.create_local_default("db", "users");

type user = {
  email: string,
  firstname: string,
  lastname: string,
};

/* GraphQL User schema */
let user =
  Schema.(
    obj("User", ~fields=user =>
      [
        field("email", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), p) =>
          p.email
        ),
        field(
          "firstname", ~args=Arg.[], ~typ=non_null(string), ~resolve=((), p) =>
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
      ~query_name="Query",
      [
        /* Query fields */
        io_field(
          "users",
          ~args=Arg.[],
          ~typ=non_null(list(non_null(user))),
          /* users resolver */
          ~resolve=((), ()) =>
          Mongo.find(usersCollection)
          |> MongoReply.get_document_list
          |> List.map(doc => {
               let profile =
                 doc |> Bson.get_element("profile") |> Bson.get_doc_element;
               {
                 email: Bson.get_element("email", doc) |> Bson.get_string,
                 firstname:
                   profile |> Bson.get_element("firstname") |> Bson.get_string,
                 lastname:
                   profile |> Bson.get_element("lastname") |> Bson.get_string,
               };
             })
          |> Lwt_result.return
        ),
      ],
    )
  );

let start = () => Server.start(~ctx=req => (), schema) |> Lwt_main.run;