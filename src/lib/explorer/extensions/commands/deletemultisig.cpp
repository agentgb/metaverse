/**
 * Copyright (c) 2016-2017 mvs developers 
 *
 * This file is part of metaverse-explorer.
 *
 * metaverse-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <metaverse/explorer/dispatch.hpp>
#include <metaverse/explorer/extensions/commands/deletemultisig.hpp>
#include <metaverse/explorer/extensions/command_extension_func.hpp>
#include <metaverse/explorer/extensions/command_assistant.hpp>
#include <metaverse/explorer/extensions/exception.hpp>

namespace libbitcoin {
namespace explorer {
namespace commands {

console_result deletemultisig::invoke (std::ostream& output,
        std::ostream& cerr, libbitcoin::server::server_node& node)
{
    auto& blockchain = node.chain_impl();
    // parameter account name check
    auto acc = blockchain.is_account_passwd_valid(auth_.name, auth_.auth);
    //auto acc_multisig = acc->get_multisig();
    account_multisig acc_multisig;
        
    if(!(acc->get_multisig_by_address(acc_multisig, option_.address)))
        throw multisig_notfound_exception{option_.address + std::string(" multisig record not found.")};
    
    acc->remove_multisig(acc_multisig);

    // change account type
    acc->set_type(account_type::common);
    if(acc->get_multisig_vec().size())
        acc->set_type(account_type::multisignature);
    // flush to db
    blockchain.store_account(acc);

    Json::Value root, pubkeys;

    root["index"] = +acc_multisig.get_index();
    root["m"] = +acc_multisig.get_m();
    root["n"] = +acc_multisig.get_n();
    root["self-publickey"] = acc_multisig.get_pubkey();
    root["description"] = acc_multisig.get_description();

    for(auto& each : acc_multisig.get_cosigner_pubkeys()) {
        pubkeys.append(each);
    }
    root["public-keys"] = pubkeys;
    root["multisig-script"] = acc_multisig.get_multisig_script();
    root["address"] = acc_multisig.get_address();
    
    // delete account address
    auto vaddr = blockchain.get_account_addresses(auth_.name);
    if(!vaddr) throw address_list_empty_exception{"empty address list for this account"};

    blockchain.delete_account_address(auth_.name);
    for (auto it = vaddr->begin(); it != vaddr->end();) {
        if (it->get_address() == acc_multisig.get_address()) {
            it = vaddr->erase(it);
            break;
        }
        ++it;
    }

    // restore address
    for (auto& each : *vaddr) {
        auto addr = std::make_shared<bc::chain::account_address>(each);
        blockchain.store_account_address(addr);
    }
    
    output << root.toStyledString();
    
    return console_result::okay;
}


} // namespace commands
} // namespace explorer
} // namespace libbitcoin

