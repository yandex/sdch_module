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
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1
--- response
FOO

=== TEST 2: text/plain is not encoded
--- config
location /sdch {
  sdch on;
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
--- response
FOO

=== TEST 3: text/html without dictionary
--- config
location /sdch {
  sdch on;
  default_type text/html;
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
--- response
FOO


=== TEST 4: text/html with wrong dictionary
--- config
location /sdch {
  sdch on;
  default_type text/html;
  return 200 "FOO";
}
--- request
GET /sdch HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: foobar1
--- response_headers
X-Sdch-Encode: 0
--- response
FOO

=== TEST 4: Offer dictionary (simple)
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/dict1.dict;
  sdch_url /sdch/dict1.dict;
  default_type text/html;
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
--- response_headers
Get-Dictionary: /sdch/dict1.dict
! X-Sdch-Encode
--- response
FOO

=== TEST 5: Offer dictionary (old dictionary)
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/dict1.dict;
  sdch_url /sdch/dict1.dict;
  default_type text/html;
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
Avail-Dictionary: foobar1
--- response_headers
Get-Dictionary: /sdch/dict1.dict
X-Sdch-Encode: 0
--- response
FOO
=== TEST 6: Do not send Get-Dictionary when downloading a dictionary
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/dict1.dict;
  sdch_url /sdch/dict1.dict;
  default_type application/x-sdch-dictionary;
  return 200 "FOO";
}

--- user_files
>>> sdch/dict1.dict
Host: example.com

THE DICTIONARY

--- request
GET /sdch/dict1.dict HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
--- response_headers
! Get-Dictionary
! X-Sdch-Encode
--- response
FOO
=== TEST 7: Do not send Get-Dictionary when downloading a dictionary (with another dictionary available)
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/dict1.dict;
  sdch_url /sdch/dict1.dict;
  default_type application/x-sdch-dictionary;
  return 200 "FOO";
}

--- user_files
>>> sdch/dict1.dict
Host: example.com

THE DICTIONARY

--- request
GET /sdch/dict1.dict HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: foobar1
--- response_headers
! Get-Dictionary
X-Sdch-Encode: 0
--- response
FOO
=== TEST 8: Offer dictionary (sdch is'n last encoding)
--- config
location /sdch {
  sdch on;
  sdch_dict $TEST_NGINX_SERVROOT/html/sdch/dict1.dict;
  sdch_url /sdch/dict1.dict;
  default_type text/html;
  return 200 "FOO";
}

--- user_files
>>> sdch/dict1.dict
Host: example.com

THE DICTIONARY

--- request
GET /sdch HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch, br
--- response_headers
Get-Dictionary: /sdch/dict1.dict
--- response
FOO
