declare module "web/fetch" {
  import {URL, URLSearchParams} from "url"
  import Headers from "headers"

  class Response {
    get bodyUsed(): boolean;
    get headers(): Headers;
    get ok(): boolean;
    get redirected(): boolean;
    get status(): number;
    get statusText(): string;
    get url(): string;
    arrayBuffer(): Promise<ArrayBuffer>;
    json(): Promise<any>;
    text(): Promise<string>;
  }

  interface RequestInit {
    method?: string;
    headers?: Headers | Record<string, string>;
    body?: string | ByteBuffer | URLSearchParams;
  }

  function fetch(resource: string | URL, options?: RequestInit): Promise<Response>;

  export { fetch };
  export default fetch;
}
