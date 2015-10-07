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

=== TEST 1: Declare AUTOAUTO support
--- config
location /sdch {
  sdch on;
  sdch_quasi on;
  default_type text/html;
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: AUTOAUTO
--- response
FOO
--- response_headers
X-Sdch-Use-As-Dictionary: 1

