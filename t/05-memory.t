use Test::Nginx::Socket no_plan;
use Test::More;

# We'll run multiple tests against same config.
# So just generate it for every test programatically.
my $servroot = $Test::Nginx::Socket::ServRoot;
$ENV{TEST_NGINX_SERVROOT} = $servroot;

add_block_preprocessor(sub {
    my $block = shift;
    $block->set_value(http_config => "
        client_body_temp_path $servroot/client_temp;
        proxy_temp_path $servroot/proxy_temp;
        fastcgi_temp_path $servroot/fastcgi_temp;
        uwsgi_temp_path $servroot/uwsgi_temp;
        scgi_temp_path $servroot/scgi_temp;

        sdch on;
        sdch_dict $servroot/html/sdch/css.dict css 1;
        sdch_dict $servroot/html/sdch/js.dict js 1;
        sdch_dict $servroot/html/sdch/css_old.dict css 2;
        sdch_dict $servroot/html/sdch/js_old.dict js 2;
        sdch_types text/css application/x-javascript;
      ");

    # Ids are:
    # user lHudK8d3 server iNm9gxBj
    # user SmjAzMQH server ATySZvFd
    # user vhNmADcl server X3GNudmJ
    # user Ct4_LIs5 server G31JTg-z
    $block->set_value(user_files => '
        >>> sdch/css.dict
        Path: /sdch

        THE CSS DICTIONARY

        >>> sdch/js.dict
        Path: /sdch

        THE JS DICTIONARY

        >>> sdch/css_old.dict
        Path: /sdch

        THE OLD CSS DICTIONARY

        >>> sdch/js_old.dict
        Path: /sdch

        THE OLD JS DICTIONARY

      ');

    return $block;
  });


repeat_each(1);
no_shuffle();
run_tests();

__DATA__

=== TEST 1: Small responses are not encoded
--- config
location /sdch/foo.css {
  sdch_url /sdch/css.dict;
  sdch_group css;
  sdch_min_length 16;
  default_type text/css;
  return 200 "FOO";
}

--- request
GET /sdch/foo.css HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: lHudK8d3

--- response_headers
! Content-Encoding


=== TEST 1: Large responses are encoded
--- config
location /sdch/foo.css {
  sdch_url /sdch/css.dict;
  sdch_group css;
  sdch_min_length 16;
  default_type text/css;
  return 200 "FOO FOO FOO FOO FOO FOO FOO FOO FOO";
}

--- request
GET /sdch/foo.css HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: lHudK8d3

--- response_headers
Content-Encoding: sdch


