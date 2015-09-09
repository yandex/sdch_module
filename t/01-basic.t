use Test::Nginx::Socket no_plan;
use Test::More;

print "ServRoot: ", $Test::Nginx::Socket::ServRoot, "\n";
$ENV{TEST_NGINX_SERVROOT} = $Test::Nginx::Socket::ServRoot;

repeat_each(2);
no_shuffle();
run_tests();

done_testing();

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
