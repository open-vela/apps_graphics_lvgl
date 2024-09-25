/*
 * Copyright (c) 2020 - 2023 the ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "../../lv_conf_internal.h"
#if LV_USE_THORVG_INTERNAL

#include "tvgMath.h"
#include "tvgRender.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/


/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

void RenderTransform::override(const Matrix& matrix)
{
    this->m = matrix;
    overriding = true;
}


bool RenderTransform::update()
{
    if (overriding) return true;

    //Init Status
    if (mathZero(x) && mathZero(y) && mathZero(degree) && mathEqual(scale, 1)) return false;

    mathIdentity(&m);

    mathScale(&m, scale, scale);

    if (!mathZero(degree)) mathRotate(&m, degree);

    mathTranslate(&m, x, y);

    return true;
}


RenderTransform::RenderTransform()
{
}


RenderTransform::RenderTransform(const RenderTransform* lhs, const RenderTransform* rhs)
{
    m = mathMultiply(&lhs->m, &rhs->m);
}

#endif /* LV_USE_THORVG_INTERNAL */

