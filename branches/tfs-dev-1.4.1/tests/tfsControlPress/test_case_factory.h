/*
 * (C) 2007-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id$
 *
 * Authors:
 *   mingyan.zc@taobao.com
 *      - initial release
 *
 */

#ifndef TEST_CASE_FACTORY_H
#define TEST_CASE_FACTORY_H

#include <string>

class TestTfsCase;

class TestCaseFactory 
{
  public:
    static TestTfsCase *getTestCase(string testIndex);
    
};

#endif
