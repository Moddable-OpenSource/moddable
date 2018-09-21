class Request {
    constructor(path, method){
        this.path = path
        this.method = method
        this.headers = []
    }
    header(key, value){
        this.headers[key] = value
    }
    data(body){
        if (this.headers['content-type'] === 'application/json'){
            this.json = JSON.parse(body)
        }
        this.body = body
    }
}

class Response {
    constructor(){
        this.data = {}
    }
    send(data){
        if (typeof data === 'string'){
            this.data.body = data
        } else {
            this.data.headers = ['Content-type', 'application/json']
            this.data.body = JSON.stringify(data)
        }
    }
    status(status){
        this.data.status = status
    }
    toObject(){
        return this.data
    }
}

class Router {
    constructor(server, config={}){
        this.server = server;
        this.config = config;
        this.routes = {};
        this.init()
    }
    init(){
        const router = this
        let request, response;
        this.server.callback = function(code, ...values){
            if (code === 2){
                request = new Request(...values);
            } else if (code === 3){
                router.log('header', ...values);
                request.header(...values);
            } else if (code === 4){
                return String;
            } else if (code === 6){
                router.log('body', values[0]);
                request.data(values[0]);
            } else if (code === 8){
                response = new Response();
                router.handle(request, response);
                return response.toObject();
            } else if (code === 10){
                router.log('request complete');
            }
        }
    }
    log(...args){
        if (this.config.debug){
            trace(args.join(' '), '\n');
        }
    }
    route(path, method, handler){
        path = path.toLowerCase();
        method = method.toUpperCase();
        this.routes[path] = this.routes[path] || {};
        this.routes[path][method] = handler;
    }
    post(path, handler){
        this.route(path, 'post', handler);
    }
    get(path, handler){
        this.route(path, 'get', handler);
    }
    handle(request, response){
        this.log('handle', request.path, request.method);
        if ((request.path in this.routes) && (request.method in this.routes[request.path])){
            this.routes[request.path][request.method](request, response);
        } else {
            response.status(500);
        }
    }
}

export default Router;