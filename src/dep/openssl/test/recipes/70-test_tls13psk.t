#! /usr/bin/env perl
# Copyright 2017 The OpenSSL Project Authors. All Rights Reserved.
#
# Licensed under the OpenSSL license (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html

use strict;
use OpenSSL::Test qw/:DEFAULT cmdstr srctop_file srctop_dir bldtop_dir/;
use OpenSSL::Test::Utils;
use File::Temp qw(tempfile);
use TLSProxy::Proxy;

my $test_name = "test_tls13psk";
setup($test_name);

plan skip_all => "TLSProxy isn't usable on $^O"
    if $^O =~ /^(VMS|MSWin32)$/;

plan skip_all => "$test_name needs the dynamic engine feature enabled"
    if disabled("engine") || disabled("dynamic-engine");

plan skip_all => "$test_name needs the sock feature enabled"
    if disabled("sock");

plan skip_all => "$test_name needs TLSv1.3 enabled"
    if disabled("tls1_3");

$ENV{OPENSSL_ia32cap} = '~0x200000200000000';
$ENV{CTLOG_FILE} = srctop_file("test", "ct", "log_list.conf");

my $proxy = TLSProxy::Proxy->new(
    undef,
    cmdstr(app(["openssl"]), display => 1),
    srctop_file("apps", "server.pem"),
    (!$ENV{HARNESS_ACTIVE} || $ENV{HARNESS_VERBOSE})
);

use constant {
    PSK_LAST_FIRST_CH => 0,
    ILLEGAL_EXT_SECOND_CH => 1
};

#Most PSK tests are done in test_ssl_new. This just checks sending a PSK
#extension when it isn't in the last place in a ClientHello

#Test 1: First get a session
(undef, my $session) = tempfile();
$proxy->clientflags("-sess_out ".$session);
$proxy->sessionfile($session);
$proxy->start() or plan skip_all => "Unable to start up Proxy for tests";
plan tests => 4;
ok(TLSProxy::Message->success(), "Initial connection");

#Test 2: Attempt a resume with PSK not in last place. Should fail
$proxy->clear();
$proxy->clientflags("-sess_in ".$session);
$proxy->filter(\&modify_psk_filter);
my $testtype = PSK_LAST_FIRST_CH;
$proxy->start();
ok(TLSProxy::Message->fail(), "PSK not last");

#Test 3: Attempt a resume after an HRR where PSK hash matches selected
#        ciperhsuite. Should see PSK on second ClientHello
$proxy->clear();
$proxy->clientflags("-sess_in ".$session);
$proxy->serverflags("-curves P-256");
$proxy->filter(undef);
$proxy->start();
#Check if the PSK is present in the second ClientHello
my $ch2 = ${$proxy->message_list}[2];
my $ch2seen = defined $ch2 && $ch2->mt() == TLSProxy::Message::MT_CLIENT_HELLO;
my $pskseen = $ch2seen
              && defined ${$ch2->{extension_data}}{TLSProxy::Message::EXT_PSK};
ok($pskseen, "PSK hash matches");

#Test 4: Attempt a resume after an HRR where PSK hash does not match selected
#        ciphersuite. Should not see PSK on second ClientHello
$proxy->clear();
$proxy->clientflags("-sess_in ".$session);
$proxy->filter(\&modify_psk_filter);
$proxy->serverflags("-curves P-256");
$proxy->cipherc("TLS13-AES-128-GCM-SHA256:TLS13-AES-256-GCM-SHA384");
$proxy->ciphers("TLS13-AES-256-GCM-SHA384");
#We force an early failure because TLS Proxy doesn't actually support
#TLS13-AES-256-GCM-SHA384. That doesn't matter for this test though.
$testtype = ILLEGAL_EXT_SECOND_CH;
$proxy->start();
#Check if the PSK is present in the second ClientHello
$ch2 = ${$proxy->message_list}[2];
$ch2seen = defined $ch2 && $ch2->mt() == TLSProxy::Message::MT_CLIENT_HELLO;
$pskseen = $ch2seen
           && defined ${$ch2->extension_data}{TLSProxy::Message::EXT_PSK};
ok($ch2seen && !$pskseen, "PSK hash does not match");


unlink $session;

sub modify_psk_filter
{
    my $proxy = shift;
    my $flight;
    my $message;

    if ($testtype == PSK_LAST_FIRST_CH) {
        $flight = 0;
    } else {
        $flight = 2;
    }

    # Only look at the first or second ClientHello
    return if $proxy->flight != $flight;

    if ($testtype == PSK_LAST_FIRST_CH) {
        $message = ${$proxy->message_list}[0];
    } else {
        $message = ${$proxy->message_list}[2];
    }

    return if (!defined $message
               || $message->mt != TLSProxy::Message::MT_CLIENT_HELLO);

    if ($testtype == PSK_LAST_FIRST_CH) {
        $message->set_extension(TLSProxy::Message::EXT_FORCE_LAST, "");
    } else {
        #Deliberately break the connection
        $message->set_extension(TLSProxy::Message::EXT_SUPPORTED_GROUPS, "");
    }
    $message->repack();
}
