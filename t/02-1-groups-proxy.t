use Test::Nginx::Socket no_plan;
use Test::More;

# We'll run multiple tests against same config.
# So just generate it for every test programatically.
my $servroot = $Test::Nginx::Socket::ServRoot;
my $servport = $Test::Nginx::Socket::ServerPort;
$ENV{TEST_NGINX_SERVROOT} = $servroot;

add_block_preprocessor(sub {
    my $block = shift;
    $block->set_value(http_config => "
        client_body_temp_path $servroot/client_temp;
        proxy_temp_path $servroot/proxy_temp;
        fastcgi_temp_path $servroot/fastcgi_temp;
        uwsgi_temp_path $servroot/uwsgi_temp;
        scgi_temp_path $servroot/scgi_temp;

        upstream bah {
            server 127.0.0.1:$servport;
        }

        #sdch on;
        sdch_dict $servroot/html/dict/css.dict css 1;
        sdch_dict $servroot/html/dict/js.dict js 1;
        sdch_dict $servroot/html/dict/css_old.dict css 2;
        sdch_dict $servroot/html/dict/js_old.dict js 2;
        sdch_types text/css application/x-javascript;
      ");

    $block->set_value(config => '
        location /back/foo.css {
          default_type text/css;
          return 200 "FOO";
        }

        location /back/foo.js {
          default_type application/x-javascript;
          return 200 "FOO";
        }

        location /sdch/foo.css {
          proxy_pass http://bah/back/foo.css;
          sdch on;
          sdch_url /sdch/css.dict;
          sdch_group css;
        }

        location /sdch/foo.js {
          proxy_pass http://bah/back/foo.js;
          sdch on;
          sdch_url /sdch/js.dict;
          sdch_group js;
        }
      ');

    # Ids are:
    # user lHudK8d3 server iNm9gxBj
    # user SmjAzMQH server ATySZvFd
    # user vhNmADcl server X3GNudmJ
    # user Ct4_LIs5 server G31JTg-z
    $block->set_value(user_files => '
        >>> dict/css.dict
        Path: /sdch

        THE CSS DICTIONARY

        >>> dict/js.dict
        Path: /sdch

        THE JS DICTIONARY

        >>> dict/css_old.dict
        Path: /sdch

        THE OLD CSS DICTIONARY

        >>> dict/js_old.dict
        Path: /sdch

        THE OLD JS DICTIONARY

      ');

    return $block;
  });


repeat_each(1);
no_shuffle();
run_tests();

__DATA__

=== TEST 1: Correct group, priority 1
--- request
GET /sdch/foo.css HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: lHudK8d3

--- response_headers
! Get-Dictionary
Content-Encoding: sdch
! X-Sdch-Encode

=== TEST 2: Correct group, priority 2
--- request
GET /sdch/foo.css HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: vhNmADcl

--- response_headers
Get-Dictionary: /sdch/css.dict
Content-Encoding: sdch
! X-Sdch-Encode

=== TEST 3: Incorrect group (js dict)
--- request
GET /sdch/foo.css HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: sz0N_vq2

--- response_headers
Get-Dictionary: /sdch/css.dict
Content-Encoding: sdch
! X-Sdch-Encode


=== TEST 42: Some totally crappy dictionary
--- request
GET /sdch/foo.css HTTP/1.1
--- more_headers
Accept-Encoding: gzip, deflate, sdch
Avail-Dictionary: foobar1

--- response
FOO
--- response_headers
Get-Dictionary: /sdch/css.dict
X-Sdch-Encode: 0

