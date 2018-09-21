import { Server } from 'http'

class Request {
    constructor(path, method){
        this.path = path
        this.method = method
        log('request', path, method)
        this.headers = []
    }
    header(key, value){
        log('header', key, value)
        this.headers[key] = value
    }
    data(body){
        log('data', body)
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
    constructor(server, config){
        this.routes = {}
        const router = this
        const log = (...args) => config.debug && trace(args.join(' '), '\n');
        server.callback = function(code, ...values){
            if (code === 2){
                this.routerRequest = new Request(...values)
            } else if (code === 3){
                this.routerRequest.header(...values)
            } else if (code === 4){
                return String
            } else if (code === 6){
                log('body', values[0]);
                this.routerRequest.data(values[0])
            } else if (code === 8){
                const response = new Response()
                router.handle(this.routerRequest, response)
                return response.toObject()
            } else if (code === 10){
                log('request complete');
            }
        }
    }
    request(path, method, handler){
        path = path.toLowerCase();
        method = method.toUpperCase();
        this.routes[path] = this.routes[path] || {}
        this.routes[path][method] = handler
    }
    post(path, handler){
        this.request(path, 'post', handler)
    }
    get(path, handler){
        this.request(path, 'get', handler)
    }
    handle(request, response){
        log('handle', request.path, request.method);
        if ((request.path in this.routes) && (request.method in this.routes[request.path])){
            this.routes[request.path][request.method](request, response)
        } else {
            response.status(500)
        }
    }
}

export default Router