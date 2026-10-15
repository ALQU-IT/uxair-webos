#!/usr/bin/env python3
"""Create a development-only, exact-source-checked UxPlay request probe copy."""
import hashlib
import pathlib
import sys

PINNED_BLOBS = {
    'raop.c': 'b6fc8f3ca31190900c64660a6e6c970381f5b48c',
    'http_request.c': 'ec030c4a1dd5f10597f44b003c2adf079288c1ea',
    'httpd.c': '56a3742f5f9e1bb1ccbcc2879b4663d2e747c726',
}
INCLUDE = '#include "raop.h"\n'
GATE = '    if (!cseq && !raop->hls_support && !ble) {\n'
PROBE = '''        int probe_body_length = -1;
        (void) http_request_get_data(request, &probe_body_length);
        const char *probe_type = http_request_get_header(request, "Content-Type");
        int probe_session = http_request_get_header(request, "X-Apple-Session-ID") != NULL;
        if (direct_info_eligible(method, url, protocol, probe_type,
                                 probe_body_length, probe_session)) {
            /* Development-only handshake experiment. Reuse the pinned RTSP
             * binary-plist info writer and its current DNS-SD feature mask.
             * Do not enter the HLS dispatcher or change connection ownership. */
            *response = http_response_create();
            if (*response) {
                char *info_data = NULL;
                int info_length = 0;
                http_response_init(*response, "HTTP/1.1", 200, "OK");
                raop_handler_info(conn, request, *response, &info_data, &info_length);
                if (info_data && info_length > 0) {
                    http_response_add_header(*response, "Server", "AirTunes/"GLOBAL_VERSION);
                    http_response_finish(*response, info_data, info_length);
                    free(info_data);
                    direct_request_diag_log_result(method, url, probe_type, protocol,
                                                   probe_body_length, probe_session, 200);
                    return;
                }
                free(info_data);
                http_response_init(*response, "HTTP/1.1", 500, "Internal Server Error");
                http_response_finish(*response, NULL, 0);
                direct_request_diag_log_result(method, url, probe_type, protocol,
                                               probe_body_length, probe_session, 500);
                return;
            }
        }
        direct_request_diag_log(method, url, probe_type, protocol,
                                probe_body_length, probe_session);
'''


def replace_region(source, start, end, replacement):
    if source.count(start) != 1 or source.count(end) != 1:
        raise ValueError('pinned UxPlay HTTP logging site changed; refuse to instrument')
    first = source.index(start)
    last = source.index(end, first + len(start))
    return source[:first] + replacement + source[last:]


def instrument_raop(source):
    if source.count(INCLUDE) != 1 or source.count(GATE) != 1:
        raise ValueError('pinned UxPlay request gate changed; refuse to instrument')
    return source.replace(INCLUDE, INCLUDE + '#include "direct_request_diag.h"\n', 1).replace(
        GATE, GATE + PROBE, 1)


def instrument_http_request(source):
    anchor = '    request->data = realloc(request->data, request->datalen + length);\n'
    if source.count(anchor) != 1:
        raise ValueError('pinned UxPlay HTTP body parser changed; refuse to instrument')
    # This also prevents int overflow in request->datalen before realloc.
    guard = ('    if (request->datalen < 0 || length > 33554432U ||\n'
             '        (size_t) request->datalen > 33554432U - length) {\n'
             '        direct_request_diag_body_limit();\n'
             '        return -1;\n'
             '    }\n')
    return '#include "direct_request_diag.h"\n' + source.replace(anchor, guard + anchor, 1)


def instrument_httpd(source):
    source = replace_region(
        source,
        '                if (recv_datalen && logger_debug) {\n',
        '                if (recv_datalen == 0) {\n',
        '                if (recv_datalen && logger_debug)\n'
        '                    logger_log(httpd->logger, LOGGER_DEBUG, "reverse HTTP response received (content omitted)");\n')
    source = replace_region(
        source,
        '            if (http_request_has_error(connection->request)) {\n',
        '            /* If request is finished, process and deallocate */\n',
        '            if (http_request_has_error(connection->request)) {\n'
        '                logger_log(httpd->logger, LOGGER_ERR, "httpd parse error (request data omitted)");\n'
        '                httpd_remove_connection(httpd, connection, 0);\n'
        '                continue;\n'
        '            }\n\n')
    source = replace_region(
        source,
        '                // Callback the received data to raop\n',
        '                httpd->callbacks.conn_request(connection->user_data, connection->request, &response);\n',
        '                // Request metadata is emitted only by the sanitized probe at the AirPlay gate.\n')
    return source


def main():
    if len(sys.argv) != 3:
        raise SystemExit('Usage: instrument-uxplay.py PINNED_LIB_DIR OUTPUT_DIR')
    source_dir, output_dir = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
    # Verify every input before writing any generated source.
    originals = {}
    for name, expected in PINNED_BLOBS.items():
        source = (source_dir / name).read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(source)).encode() + b'\0' + source).hexdigest()
        if blob != expected:
            raise SystemExit(f'UxPlay {name} is not the pinned source: {blob}')
        originals[name] = source.decode('utf-8')
    generated = {
        'raop.c': instrument_raop(originals['raop.c']),
        'http_request.c': instrument_http_request(originals['http_request.c']),
        'httpd.c': instrument_httpd(originals['httpd.c']),
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    for name, source in generated.items():
        (output_dir / name).write_text(source, encoding='utf-8', newline='\n')
    print('Development-only direct request probe and HTTP safety guards generated from pinned UxPlay sources')


if __name__ == '__main__':
    main()
