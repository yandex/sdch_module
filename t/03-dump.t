use Test::Nginx::Socket no_plan;
use Test::More;

my $servroot = $Test::Nginx::Socket::ServRoot;
$ENV{TEST_NGINX_SERVROOT} = $servroot;

add_block_preprocessor(sub {
    my $block = shift;
    $block->set_value('http_config',
      "
        client_body_temp_path $servroot/client_temp;
        proxy_temp_path $servroot/proxy_temp;
        fastcgi_temp_path $servroot/fastcgi_temp;
        uwsgi_temp_path $servroot/uwsgi_temp;
        scgi_temp_path $servroot/scgi_temp;
      ");
    return $block;
  });


repeat_each(2);
no_shuffle();
run_tests();

__DATA__

=== TEST 1: Sanity
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/dict1.dict;
  sdch_url /sdch/dict1.dict;
  default_type text/html;
  sdch_dumpdir $TEST_NGINX_SERVROOT/client_temp;
  return 200 "FOO";
}
--- user_files
>>> sdch/dict1.dict
Host: example.com

THE DICTIONARY

--- request
GET /sdch HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: WSsxLmBh

--- grep_error_log chop
dump open file
--- grep_error_log_out
dump open file
