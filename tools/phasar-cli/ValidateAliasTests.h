/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_CLI_VALIDATE_ALIAS_TESTS_H
#define PHASAR_CLI_VALIDATE_ALIAS_TESTS_H

namespace psr {
class HelperAnalyses;
}

namespace psr {

/// Run alias-check validation: scan the module for MAYALIAS/NOALIAS/MUSTALIAS
/// (and PARTIALALIAS) calls, query the built alias model, and print SUCCESS/FAILURE.
/// \return false if any check failed (caller should exit(1)).
bool runValidateAliasTests(HelperAnalyses &HA);

} // namespace psr

#endif // PHASAR_CLI_VALIDATE_ALIAS_TESTS_H
