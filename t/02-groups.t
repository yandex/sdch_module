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
        sdch_types text/css application/x-javascript;
      ");

    $block->set_value(config => '
        location /sdch/foo.css {
          sdch_url /sdch/css.dict;
          sdch_group css;
          default_type text/css;
          return 200 "FOO";
        }

        location /sdch/foo.js {
          sdch_url /sdch/js.dict;
          sdch_group js;
          default_type application/x-javascript;
          return 200 "FOO";
        }
      ');

    $block->set_value(user_files => '
        >>> sdch/css.dict
        Path: /sdch

        THE CSS DICTIONARY

        >>> sdch/js.dict
        Path: /sdch

        THE JS DICTIONARY

      ');

    return $block;
  });


repeat_each(1);
no_shuffle();
run_tests();

__DATA__

=== TEST 42: Overall
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

