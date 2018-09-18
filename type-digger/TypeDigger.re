
Printexc.record_backtrace(true);
open Lib;
module Json = Vendor.Json;
open DigTypes;
/* Log.spamError := true; */

let toJson = (base, src, name) => {
  let state = TopTypes.forRootPath(base);
  let uri = Utils.toUri(Filename.concat(base, src));
  let%try tbl = GetTypeMap.forInitialType(~state, uri, name);

  let json = Rpc.J.o(Hashtbl.fold(((moduleName, path, name), v, items) => {
    [(moduleName ++ ":" ++ String.concat(".", path) ++ ":" ++ name, 
    SerializeSimplerType.declToJson(SerializeSimplerType.sourceToJson, v)
    ), ...items]
  }, tbl, []));
  Files.writeFile("out.json", Json.stringifyPretty(json)) |> ignore;
  /* print_endline(); */
  Ok(())
};

let toSerializer = (base, src, name, dest) => {
  let state = TopTypes.forRootPath(base);
  let uri = Utils.toUri(Filename.concat(base, src));
  let%try package = State.getPackage(uri, state);
  let%try tbl = GetTypeMap.forInitialType(~state, uri, name);

  let decls = Hashtbl.fold(((moduleName, modulePath, name), decl, bindings) => {
    [MakeSerializer.decl(
      MakeSerializer.sourceTransformer,
      ~moduleName,
      ~modulePath,
      ~name,
      decl
    ), ...bindings]
  }, tbl, []);

  Pprintast.structure(Format.str_formatter, [Ast_helper.Str.value(
    Recursive,
    decls
  )]);
  let ml = Format.flush_str_formatter();
  let%try text = switch (package.refmtPath) {
    | None => Ok(ml)
    | Some(refmt) =>
    Lib.AsYouType.convertToRe(~formatWidth=Some(100),
      ~interface=false,
      ml,
      refmt
      )
  };
  Files.writeFile(dest, text) |> ignore;
  Ok(())
};

switch (Sys.argv) {
  | [|_, src, name, dest|] => {
    switch (toSerializer(Sys.getcwd(), src, name, dest)) {
      | Result.Ok(()) => print_endline("Success")
      | Result.Error(message) => print_endline("Failed: " ++ message)
    }
  }
  | _ => failwith("Bad args")
}