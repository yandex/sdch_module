# Keep nginx running between tests. We have to preserve quasi dictionaries on
# server. We have to set it before loading Test::Nginx
BEGIN {
$ENV{TEST_NGINX_FORCE_RESTART_ON_TEST} = '0';
}

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
    $block->set_value(user_files => 
'>>> sdch/test.dict
Path: /sdch

THE TEST DICTIONARY
'
    );

    return $block;
  });


repeat_each(1);
no_shuffle();
run_tests();


__DATA__

=== TEST 1: Don't ask for Vary
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/test.dict;
  sdch_vary off;
  default_type text/html;
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1

--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: FSZ191ye

--- response
FOO
--- response_headers
Content-Encoding: sdch
! Vary

=== TEST 2: Ask to add Vary
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/test.dict;
  sdch_vary on;
  default_type text/html;
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1

--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: FSZ191ye

--- response
FOO
--- response_headers
Content-Encoding: sdch
Vary: Accept-Encoding, Avail-Dictionary

