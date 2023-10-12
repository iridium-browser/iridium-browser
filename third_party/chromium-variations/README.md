# Chromium Variations README

## Overview

This repository contains various directories, each of which holds a seed.json
file. Each seed.json file contains a variations seed. A variations seed is used
to turn on/off field trials and features in Chromium projects, such as the
Chromium browser.

Changes to this repository will not be accepted.

## Seed Files

*   `single_group_per_study_prefer_existing_behavior/*`

    For each study, this seed enables the largest field trial group. If there
    are multiple groups having the same largest size for a given study, prefer a
    group which enables existing behaviour.

*   `single_group_per_study_prefer_new_behavior/*`

    For each study, this seed enables the largest field trial group. If there
    are multiple groups having the same largest size for a given study, prefer a
    group which enables new behaviour.

## Using Seed Files

The seeds in this repo can be consumed by Chromium M115 and higher.

To launch Chromium using a given seed, include the following command-line
parameters in the launch command.

*   `--variations-test-seed-path=${PATH_TO_SEED_JSON_FILE}`

    This argument causes Chromium to load ${PATH_TO_SEED_JSON_FILE} and use it
    for field trial assignments.

*   `--fake-variations-channel=${CHANNEL}`

    This argument forces Chrome to use the configuration for the given channel
    when applying the seed. The allowed ${CHANNEL} values are: stable, beta, dev
    and canary. Each seed contains group and feature assignments for all
    platforms and channels.

*   `--accept-empty-variations-seed-signature`

    This argument permits to Chrome to accept a variations seed .json file where
    the seed signature is empty.
