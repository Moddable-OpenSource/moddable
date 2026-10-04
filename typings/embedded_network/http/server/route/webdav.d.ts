/*
* Copyright (c) 2026 Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK Tools.
*
*   The Moddable SDK Tools is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   The Moddable SDK Tools is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
*
*/

// a WebDAV (RFC 4918) server over a directory. It composes with other routes: onRoute returns handlers for
// requests under prefix, and undefined for the rest
declare module "embedded:network/http/server/route/webdav" {
    import type { HTTPConnectionHandlers, HTTPRequest } from "embedded:network/http/server";
    import type { Directory } from "embedded:storage/files";

    export interface WebDAVOptions {
        directory: Directory;		// the directory to serve
        prefix?: string;			// request path under which the directory is served, such as "/dav"; defaults to ""
        log?: boolean;				// trace requests; defaults to false
        appleDouble?: boolean;		// accept macOS "._" metadata files; defaults to true
    }

    class WebDAV {
        constructor(options: WebDAVOptions);
        onRoute(request: HTTPRequest): HTTPConnectionHandlers | undefined;
        readonly directory: Directory;
        readonly prefix: string;
    }

    export default WebDAV;
}
