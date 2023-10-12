<?php
/**
 * Copyright (c) 2021, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 *
 *
 * Can be run with "php avifinfo_test.php" from this folder.
 */

require_once('../avifinfo.php');

$features = array( 'width'        => false, 'height'       => false,
                   'bit_depth'    => false, 'num_channels' => false );
$handle = fopen( 'avifinfo_test_1x1.avif', 'rb' );
if ( $handle ) {
  $parser  = new Avifinfo\Parser( $handle );
  $success = $parser->parse_ftyp() && $parser->parse_file();
  fclose( $handle );
  if ( $success ) {
    $features = $parser->features->primary_item_features;
  }
}

if ( !$features['width'] || !$features['height'] ||
     !$features['bit_depth'] || !$features['num_channels'] ) {
  echo 'Failure'.PHP_EOL;
} else {
  echo 'Success'.PHP_EOL;
}
