import { Server } from "http";
import Router from "router";

const server = new Server({ port: 8080 });
const router = new Router(server, { debug: true });

router.get("/", (request, response) => {
    response.send(`Hello ${request.path}`);
});

router.post("/", (request, response) => {
    response.send({
        message: "I got some data",
        data: request.json,
    });
});

