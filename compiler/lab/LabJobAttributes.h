// Copyright (c) Chemical Language Foundation 2026.

//
// Created by wakaztahir on 6/13/26.
//

#pragma once

/**
 * fast copyable intN like attributes present on each job
 */
struct LabJobAttributes {

    /**
     * this means only the remote dependencies of the job will be downloaded
     * no typechecking, compilation or linking stage will take place
     */
    bool download_only = false;

    /**
     * this means typechecking stage will be performed, rest of the stages will be skipped
     */
    bool check_only = false;

};